#!/usr/bin/env python3
"""Pin seven active source validators, controls and their two hardened helpers."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import (
    check_row, digest, invoke, reject, require,
)


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/parity/source_driver_cohort_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "d22d29cc0471b1fb6b66fcaf3ade5b08bf7e76d076acc87d767b624594e27796"
WITNESS = ROOT / "docs/parity/source_driver_cohort_optimized_preedit_5_0_6.json.gz"
HELPER_WITNESS = ROOT / "docs/parity/source_driver_cohort_helper_preedit_5_0_6.json.gz"
NAMES = {"registry", "query_store", "private_runtime", "entity_api", "pagination",
         "runtime_protocol", "runtime_bridge"}


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_bindings(bindings, registered):
    require(len(bindings) == 7 and {row["name"] for row in bindings} == NAMES,
            "Seven active source-validator families")
    for row in bindings:
        require(registered[row["test"]]["command"] == row["command"]
                and registered[row["controls_test"]]["command"] == row["controls_command"],
                "Source/control CTest registration changed: " + row["name"])
        require(digest((ROOT / row["contracts"]).read_bytes()) == row["contract_sha256"]
                and digest((ROOT / row["execution"]).read_bytes()) == row["execution_sha256"],
                "Immutable contract or execution changed: " + row["name"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve existing control evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Source transition changed")
    manifest, witness, helpers = read(MANIFEST), read(WITNESS), read(HELPER_WITNESS)
    require(manifest["complete"] is True and manifest["installed"] is True
            and manifest["whole_goal_complete"] is False
            and manifest["scripts"] == 16 and manifest["assertions"] == 570
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and manifest["helper_witness_sha256"] == digest(HELPER_WITNESS.read_bytes())
            and witness["complete"] is True and helpers["complete"] is True
            and witness["mutation"] == {"complete": False}, "Bounded source transition scope")
    require({row["name"] for row in witness["outcomes"]} == NAMES
            and all([(item["mode"], item["optimized"], item["exit_code"] == 0)
                     for item in row["results"]]
                    == [("valid", False, True), ("incomplete", False, False),
                        ("incomplete", True, True)] for row in witness["outcomes"]),
            "Seven actual pre-edit incomplete-record false acceptances")
    require([(row["label"], row["optimized"], row["exit_code"] == 0)
             for row in helpers["results"]] == [
                (label, optimized, optimized) for label in
                ("archive_symlink", "missing_observer_marker") for optimized in (False, True)],
            "Two actual pre-edit imported-helper false acceptances")
    originals = {}
    for row in witness["source_rows"] + helpers["source_rows"]:
        require(row["path"] not in originals or originals[row["path"]] == row,
                "Inconsistent original source snapshot")
        originals[row["path"]] = row
    require(len(manifest["rows"]) == len(originals) == 16
            and {row["historical_path"] for row in manifest["rows"]} == set(originals),
            "Complete old/current source pairs")
    for row in manifest["rows"]:
        check_row(row, originals[row["historical_path"]])
    require(sum(row["assertions"] for row in manifest["rows"]) == 570,
            "Complete unconditional check inventory")
    for name, expected in manifest["dependencies"].items():
        require(digest((ROOT / name).read_bytes()) == expected, "Source-transition helper changed")
    registry = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(ROOT / "build_native_current"), "--show-only=json-v1"],
        text=True))["tests"]
    registered = {row["name"]: row for row in registry}
    bindings = manifest["bindings"]
    check_bindings(bindings, registered)

    source_controls = []
    first = manifest["rows"][0]
    original = originals[first["historical_path"]]
    reject("changed_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), original), source_controls)
    reject("changed_current_source", lambda: check_row(
        first, original, first["after_source"] + "\n"), source_controls)
    reject("incomplete_assertion_transition", lambda: check_row(
        dict(first, assert_only_source=first["assert_only_source"] + "\npass\n"), original),
        source_controls)
    reject("missing_declared_adaptation", lambda: check_row(
        dict(first, adaptations=first["adaptations"][:-1]), original), source_controls)
    altered = copy.deepcopy(registered)
    altered[bindings[0]["test"]]["command"][1] = "/tmp/substituted_verifier.py"
    reject("substituted_registered_verifier", lambda: check_bindings(bindings, altered),
           source_controls)

    outcomes, helper_outcomes, replay_refusals = [], [], []
    with tempfile.TemporaryDirectory(prefix="teaql_source_driver_hardened_506_") as owned:
        directory = Path(owned)
        for row in bindings:
            invalid = directory / (row["name"] + ".json.gz")
            record = read(ROOT / row["execution"])
            record["complete"] = False
            invalid.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
            script = Path(row["command"][1])
            base = row["command"][2:]
            for optimized in (False, True):
                valid = invoke(script, optimized, *base)
                require(valid.returncode == 0,
                        "Valid source evidence rejected: " + row["name"] + valid.stderr[-800:])
                arguments = list(base)
                arguments[arguments.index("--execution") + 1] = str(invalid)
                incomplete = invoke(script, optimized, *arguments)
                require(incomplete.returncode != 0 and "AssertionError" in incomplete.stderr,
                        "Incomplete source evidence accepted: " + row["name"])
                outcomes.append({"name": row["name"], "optimized": optimized,
                                 "valid_exit_code": valid.returncode,
                                 "incomplete_exit_code": incomplete.returncode})
                print(row["name"], "optimized", optimized, "valid/incomplete pass", flush=True)
        scripts = directory / "scripts"
        scripts.mkdir()
        (scripts / "verify_query_stores_source_5_0_6.py").write_text("# missing source markers\n")
        for label, snippet in helpers["snippets"].items():
            snippet = snippet.replace("import archive_driver_provenance_5_0_6 as m",
                                      "import archive_driver_provenance_hardened_5_0_6 as m")
            snippet = snippet.replace("import replay_query_store_drivers_5_0_6 as m",
                                      "import replay_query_store_drivers_hardened_5_0_6 as m")
            for optimized in (False, True):
                result = invoke("-c", optimized, snippet, ROOT / "scripts", directory)
                require(result.returncode != 0 and "AssertionError" in result.stderr,
                        "Invalid imported helper input accepted: " + label)
                helper_outcomes.append({"name": label, "optimized": optimized,
                                        "exit_code": result.returncode})
        for name in ("pagination", "runtime_protocol", "runtime_bridge"):
            row = next(row for row in bindings if row["name"] == name)
            destination = directory / (name + "_replay.json.gz")
            result = invoke(row["command"][1], True, "replay", "--contracts",
                            ROOT / row["contracts"], "--record", destination)
            require(result.returncode != 0 and "replay requires assertions" in result.stderr
                    and not destination.exists(), "Optimized driver replay was not refused")
            replay_refusals.append(name)
        for filename, mode in (("archive_driver_provenance_hardened_5_0_6.py", ["replay"]),
                               ("replay_query_store_drivers_hardened_5_0_6.py", [])):
            destination = directory / (filename + ".json.gz")
            row = next(row for row in bindings if row["name"] == "pagination")
            result = invoke(ROOT / "scripts" / filename, True, *mode,
                            "--contracts", ROOT / row["contracts"], "--record", destination)
            require(result.returncode != 0 and "replay requires assertions" in result.stderr
                    and not destination.exists(), "Optimized helper replay was not refused")
            replay_refusals.append(filename)
        removed_bytes = sum(path.stat().st_size for path in directory.rglob("*") if path.is_file())
    require(not directory.exists(), "Task-owned control evidence remains")
    if args.record is not None:
        result = {"complete": True, "whole_goal_complete": False,
                  "manifest_sha256": digest(MANIFEST.read_bytes()),
                  "source_controls": source_controls, "outcomes": outcomes,
                  "helper_outcomes": helper_outcomes, "replay_refusals": replay_refusals,
                  "owned_bytes_removed": removed_bytes, "owned_directory_removed": True}
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print("Seven source validators: 570 unconditional checks, five source controls, "
          "normal/optimized evidence and two imported helper boundaries pass")


if __name__ == "__main__":
    main()
