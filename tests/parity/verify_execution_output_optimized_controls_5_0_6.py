#!/usr/bin/env python3
"""Keep execution, termination and diagnostic decisions active under Python -O."""

import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from execution_output_control_support_5_0_6 import FAMILIES, build_cases, run_case
from verify_generated_driver_hardened_sources_5_0_6 import (
    check_row, digest, reject, require,
)


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/parity/execution_output_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "36cc132c4a382a50b67ef3729b7c40f47c86cf6617c1f197008dc50768b2f0a6"
WITNESS = ROOT / "docs/parity/execution_output_optimized_preedit_5_0_6.json.gz"


def template(command, source_dir, build_dir):
    return ["${PYTHON}", *[
        part.replace(str(build_dir), "${BINARY_DIR}")
        .replace(str(source_dir), "${SOURCE_DIR}") for part in command[1:]]]


def check_binding(binding, registered, build_dir):
    require(binding["test"] in registered, "Missing execution comparator")
    command = registered[binding["test"]]["command"]
    require(Path(command[0]).resolve() == Path(sys.executable).resolve()
            and template(command, ROOT, build_dir) == binding["command"],
            "Execution comparator registration changed: " + binding["test"])


def check_sources(manifest, witness):
    originals = {row["path"]: row for row in witness["source_rows"]}
    rows = manifest["rows"]
    require(len(rows) == len(originals) == 4
            and set(originals) == {row["script"] for row in FAMILIES.values()}
            and {row["historical_path"] for row in rows} == set(originals)
            and sum(row["assertions"] for row in rows) == 11
            and all(row["adaptations"] == [] for row in rows),
            "Exact four-script assertion-only scope")
    for row in rows:
        check_row(row, originals[row["historical_path"]])
    for name, expected in manifest["dependencies"].items():
        require(digest((ROOT / name).read_bytes()) == expected,
                "Execution comparator helper or original observations changed: " + name)
    bindings = manifest["bindings"]
    require(len(bindings) == len(witness["bindings"]) == 8
            and {(row["family"], row["profile"]) for row in bindings}
            == {(name, profile) for name in FAMILIES for profile in ("native", "module")},
            "Four native and four module registrations")
    current = {row["historical_path"]: row["current_path"] for row in rows}
    previous = {(row["family"], row["profile"]): row for row in witness["bindings"]}
    for binding in bindings:
        family = FAMILIES[binding["family"]]
        old = previous[binding["family"], binding["profile"]]
        command = list(old["command"])
        suffix = "/" + family["script"]
        require(command[1].endswith(suffix)
                and old["test"] == binding["test"] == family[binding["profile"]],
                "Original execution registration identity")
        source_dir = Path(command[1][:-len(suffix)])
        build_dir = source_dir / ("build_native_current" if binding["profile"] == "native"
                                  else "build_modules_native")
        command[1] = str(source_dir / current[family["script"]])
        require(template(command, source_dir, build_dir) == binding["command"],
                "Only the active comparator path may change")
    return originals


def check_preedit(witness, cases):
    results = {(row["name"], row["optimized"]): row for row in witness["results"]}
    require(witness["complete"] is True and witness["whole_goal_complete"] is False
            and witness["owned_directory_removed"] is True
            and len(results) == len(witness["results"]) == 40,
            "Complete owned pre-edit execution witness")
    for case in cases:
        for optimized in (False, True):
            require((case["name"], optimized) in results, "Missing pre-edit fixture result")
            row = results[case["name"], optimized]
            expected = {name: False for name in case["markers"]} if (
                optimized and case["family"] == "service_errors") else case["expected_invocations"]
            require(row["family"] == case["family"] and row["valid"] == case["valid"]
                    and (row["exit_code"] == 0) == (optimized or case["valid"])
                    and row["invoked"] == expected,
                    "Pre-edit false acceptance or omitted execution changed")


def check_imported_helper(optimized):
    code = ("import sys\n"
            "sys.path.insert(0, " + repr(str(ROOT / "scripts")) + ")\n"
            "from verify_runtime_configuration_kernel_5_0_6 import "
            "load_original_record, verify_inputs\n"
            "name = 'runtime_service_errors_original_observations_5_0_6.json'\n"
            "record = load_original_record(name)\n"
            "record['cases'] = []\n"
            "verify_inputs(record, name)\n")
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1")
    environment.pop("PYTHONOPTIMIZE", None)
    environment.pop("PYTHONPATH", None)
    result = subprocess.run([sys.executable, "-B", *(["-O"] if optimized else []), "-c", code],
                            cwd=ROOT, env=environment, capture_output=True, text=True, timeout=120)
    require(result.returncode != 0 and "AssertionError" in result.stderr
            and "loaded record changed" in result.stderr,
            "Imported source validator accepted substituted observations")
    return {"optimized": optimized, "exit_code": result.returncode,
            "stdout": result.stdout, "stderr": result.stderr}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--profile", choices=("native", "module"), default="native")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve existing control evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Source transition changed")
    manifest = json.loads(gzip.decompress(MANIFEST.read_bytes()))
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(manifest["complete"] is True and manifest["installed"] is True
            and manifest["whole_goal_complete"] is False and manifest["scripts"] == 4
            and manifest["assertions"] == 11
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a",
            "Locked source-transition scope")
    originals = check_sources(manifest, witness)
    registry = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    registered = {row["name"]: row for row in registry}
    bindings = [row for row in manifest["bindings"] if row["profile"] == args.profile]
    for binding in bindings:
        check_binding(binding, registered, args.build_dir.resolve())

    controls = []
    first = manifest["rows"][0]
    reject("changed_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), controls)
    reject("changed_active_source", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), controls)
    reject("missing_assertion_replacement", lambda: check_row(
        dict(first, assert_only_source=first["before_source"]),
        originals[first["historical_path"]]), controls)
    altered = copy.deepcopy(registered)
    altered[bindings[0]["test"]]["command"][1] = str(ROOT / first["historical_path"])
    reject("old_registered_dispatcher", lambda: check_binding(
        bindings[0], altered, args.build_dir.resolve()), controls)
    altered = copy.deepcopy(registered)
    del altered[bindings[0]["test"]]
    reject("missing_registered_comparator", lambda: check_binding(
        bindings[0], altered, args.build_dir.resolve()), controls)

    scripts = {row["historical_path"]: ROOT / row["current_path"] for row in manifest["rows"]}
    observed, helpers = [], []
    with tempfile.TemporaryDirectory(prefix="teaql_execution_output_controls_506_") as owned:
        cases = build_cases(owned)
        check_preedit(witness, cases)
        for optimized in (False, True):
            for case in cases:
                result = run_case(scripts[FAMILIES[case["family"]]["script"]], case, optimized)
                require((result["exit_code"] == 0) == case["valid"]
                        and result["invoked"] == case["expected_invocations"]
                        and (case["valid"] or case["error"] in result["stderr"]),
                        "Execution fixture failed: " + case["name"] + " " + result["stderr"][-1000:])
                observed.append(result)
            helpers.append(check_imported_helper(optimized))
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob("*") if path.is_file())
    require(not Path(owned).exists(), "Owned subprocess fixtures were not removed")
    if args.record is not None:
        record = {"complete": True, "whole_goal_complete": False,
                  "profile": args.profile, "manifest_sha256": digest(MANIFEST.read_bytes()),
                  "witness_sha256": digest(WITNESS.read_bytes()), "bindings": bindings,
                  "source_controls": controls, "results": observed, "imported_helper": helpers,
                  "owned_directory_removed": True, "owned_bytes_removed": removed_bytes}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Execution comparators: 11 unconditional checks, five valid and fifteen invalid "
          "fixtures in both Python modes, imported validation and five source controls pass ("
          + args.profile + ")")


if __name__ == "__main__":
    main()
