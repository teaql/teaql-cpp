#!/usr/bin/env python3
"""Qualify active generated/SQLite comparators and their historical bindings."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from generated_cli_control_support_5_0_6 import (
    FAMILIES, MODES, owned_bytes, prepare, run_case,
)
from verify_generated_driver_hardened_sources_5_0_6 import (
    check_row, digest, require,
)


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from generated_cli_source_transition_5_0_6 import TRANSITIONS, current_comparator

MANIFEST = ROOT / "docs/parity/generated_cli_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "78a18bc33748c6b67d496e753412fa36f8f8a0ba635dfc3c7d9760929aa7bb26"
WITNESS = ROOT / "docs/parity/generated_cli_optimized_preedit_5_0_6.json.gz"


def template(command, source_dir, build_dir):
    return ["${PYTHON}", *[part.replace(str(build_dir), "${BINARY_DIR}")
                          .replace(str(source_dir), "${SOURCE_DIR}") for part in command[1:]]]


def check_binding(binding, registered, build_dir):
    require(binding["test"] in registered, "Missing generated CLI registration")
    command = registered[binding["test"]]["command"]
    require(Path(command[0]).resolve() == Path(sys.executable).resolve()
            and template(command, ROOT, build_dir) == binding["command"],
            "Generated CLI registration changed: " + binding["test"])


def rejected(label, action, controls):
    try:
        action()
    except (AssertionError, RuntimeError):
        controls.append(label)
    else:
        raise AssertionError("Corrupt generated CLI transition accepted: " + label)


def check_sources(manifest, witness):
    originals = {row["path"]: row for row in witness["source_rows"]}
    require(len(originals) == len(manifest["rows"]) == 11
            and {row["historical_path"] for row in manifest["rows"]} == set(originals)
            and sum(row["assertions"] for row in manifest["rows"]) == 26
            and sum(len(row["adaptations"]) for row in manifest["rows"]) == 18,
            "Exact comparator, validator and control source scope")
    for row in manifest["rows"]:
        check_row(row, originals[row["historical_path"]])
    for path, wanted in manifest["dependencies"].items():
        require(digest((ROOT / path).read_bytes()) == wanted,
                "Current CLI helper, corpus or immutable source contract changed: " + path)
    require(witness["support_sha256"] == digest(
        (ROOT / "tests/parity/generated_cli_control_support_5_0_6.py").read_bytes()),
        "Owned fixture recipe changed")
    for path, wanted in witness["corpora"].items():
        require(digest((ROOT / "tests/parity" / path).read_bytes()) == wanted,
                "Original retained CLI corpus changed")
    required = {"tests/parity/" + data[0] for data in FAMILIES.values()}
    require(TRANSITIONS == {row["historical_path"]: (row["before_sha256"], row["current_path"],
                                                  row["after_sha256"])
                            for row in manifest["rows"] if row["historical_path"] in required},
            "Exact seven-comparator admission table")
    previous = {(row["profile"], row["test"]): row for row in witness["bindings"]}
    require(len(previous) == len(manifest["bindings"]) == 24
            and {(row["profile"], row["test"]) for row in manifest["bindings"]} == set(previous),
            "Complete native/module registration transition")
    paths = {row["historical_path"]: row["current_path"] for row in manifest["rows"]}
    for binding in manifest["bindings"]:
        old = previous[binding["profile"], binding["test"]]
        command = list(old["command"])
        historical = next(path for path in paths if command[1].endswith("/" + path))
        source_dir = Path(command[1][:-len(historical) - 1])
        command[1] = str(source_dir / paths[historical])
        build = source_dir / ("build_native_current" if binding["profile"] == "native"
                              else "build_modules_native")
        require(template(command, source_dir, build) == binding["command"],
                "Only the active CLI path may change")
    return originals


def check_preedit(witness):
    results = {(row["family"], row["mode"], row["optimized"]): row for row in witness["results"]}
    require(witness["complete"] is True and witness["whole_goal_complete"] is False
            and witness["owned_directory_removed"] is True
            and len(results) == len(witness["results"]) == 58,
            "Complete owned pre-edit witness")
    for family in FAMILIES:
        modes = ("valid", "foreign_source", "partial", "extra")
        if family == "sqlite_raw_debug":
            modes += ("empty",)
        for mode in modes:
            for optimized in (False, True):
                require((family, mode, optimized) in results, "Missing pre-edit result")
                row = results[family, mode, optimized]
                accepted = (mode in ("valid", "empty") or mode == "foreign_source" and optimized
                            or mode in ("partial", "extra") and family != "expression_diagnostics")
                require((row["exit_code"] == 0) == accepted and row["invoked"] == accepted,
                        "Pre-edit false acceptance or execution changed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--profile", choices=("native", "module"), default="native")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve existing evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "CLI source transition changed")
    manifest = json.loads(gzip.decompress(MANIFEST.read_bytes()))
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(manifest["complete"] is True and manifest["installed"] is True
            and manifest["whole_goal_complete"] is False and manifest["scripts"] == 11
            and manifest["assertions"] == 26
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a",
            "Locked bounded CLI repair scope")
    originals = check_sources(manifest, witness)
    check_preedit(witness)
    rows = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    registered = {row["name"]: row for row in rows}
    bindings = [row for row in manifest["bindings"] if row["profile"] == args.profile]
    require(len(bindings) == (19 if args.profile == "native" else 5), "Exact profile binding scope")
    for binding in bindings:
        check_binding(binding, registered, args.build_dir.resolve())

    controls = []
    first = manifest["rows"][0]
    rejected("changed_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), controls)
    rejected("changed_active_source", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), controls)
    adapted = next(row for row in manifest["rows"] if row["adaptations"])
    rejected("missing_declared_adaptation", lambda: check_row(
        dict(adapted, adaptations=adapted["adaptations"][:-1]),
        originals[adapted["historical_path"]]), controls)
    altered = copy.deepcopy(registered)
    old_dispatch = next(row["historical_path"] for row in manifest["rows"]
                        if "${SOURCE_DIR}/" + row["current_path"] == bindings[0]["command"][1])
    altered[bindings[0]["test"]]["command"][1] = str(ROOT / old_dispatch)
    rejected("old_active_dispatcher", lambda: check_binding(
        bindings[0], altered, args.build_dir.resolve()), controls)
    scripts = {name: TRANSITIONS["tests/parity/" + data[0]][1]
               for name, data in FAMILIES.items()}
    observed = []
    with tempfile.TemporaryDirectory(prefix="teaql_generated_cli_controls_506_") as owned:
        repository, probes = prepare(owned, scripts)
        historical, (_, current, _) = next(iter(TRANSITIONS.items()))
        for relative in (historical, current):
            target = repository / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / relative).read_bytes())
        require(current_comparator(repository, historical) == current, "Valid copied source admission")
        for relative in (historical, current):
            target = repository / relative
            source = target.read_bytes()
            target.write_bytes(source + b"\n")
            rejected("changed_admitted_source:" + relative,
                     lambda: current_comparator(repository, historical), controls)
            target.write_bytes(source)
        rejected("rewritten_historical_selector", lambda: current_comparator(repository, current), controls)
        for optimized in (False, True):
            for family in FAMILIES:
                for mode in MODES:
                    row = run_case(repository, probes, scripts, family, mode, optimized)
                    require((row["exit_code"] == 0) == (mode == "valid")
                            and row["invoked"] == (mode == "valid" or mode.endswith("output"))
                            and (mode == "valid" or any(error in row["stderr"] for error in
                                 ("AssertionError", "RuntimeError", "SystemExit", "JSONDecodeError"))
                                 or row["stdout"].startswith("Case ") or row["stdout"].startswith("0 ")
                                 or row["stderr"].startswith("Case ")),
                            "CLI control failed: " + str({k: v for k, v in row.items()
                                                         if k != "stdout"})[-1500:])
                    observed.append(row)
        removed = owned_bytes(owned)
    require(not Path(owned).exists(), "Owned CLI subprocess fixtures remain")
    if args.record:
        record = {"complete": True, "whole_goal_complete": False, "profile": args.profile,
                  "manifest_sha256": digest(MANIFEST.read_bytes()), "source_controls": controls,
                  "bindings": bindings, "results": observed,
                  "owned_bytes_removed": removed, "owned_directory_removed": True}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Generated/SQLite CLI repair: 26 checks, seven exact matrices, normal/optimized "
          "valid, source, shape, cardinality and output controls pass (" + args.profile + ")")


if __name__ == "__main__":
    main()
