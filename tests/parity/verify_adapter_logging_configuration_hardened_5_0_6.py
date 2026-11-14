#!/usr/bin/env python3
"""Keep adapter, logging and configuration corruption checks active under -O."""

import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import check_row, digest, require


ROOT = Path(__file__).resolve().parents[2]
WITNESS = ROOT / "docs/parity/adapter_logging_configuration_preedit_5_0_6.json.gz"
MANIFEST = ROOT / "docs/parity/adapter_logging_configuration_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "5b7282be1102f02af57be80fd6b2df21d82448c7d2db85bac44b89b7fd317254"
BUILD_ARGUMENT = "str(root/'build_native_current')"


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_bindings(bindings, tests, build_dir):
    require(len(bindings) == 3, "Three control registrations required")
    for binding in bindings:
        require(binding["test"] in tests, "Control registration missing")
        test = tests[binding["test"]]
        command = test["command"]
        normalized = ["${PYTHON}", *[
            part.replace(str(build_dir.resolve()), "${BINARY_DIR}")
            .replace(str(ROOT), "${SOURCE_DIR}") for part in command[1:]]]
        require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                and normalized == binding["command"]
                and test["properties"] == binding["properties"],
                "Control command or fixture properties changed")


def rejected(label, action, outcomes):
    try:
        action()
    except AssertionError:
        outcomes.append(label)
    else:
        raise AssertionError("Accepted corrupt control source: " + label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve existing control evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Control transition changed")
    manifest, witness = read(MANIFEST), read(WITNESS)
    require(manifest["complete"] is True and manifest["whole_goal_complete"] is False
            and manifest["scripts"] == 3 and manifest["assertions"] == 16
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a"
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and witness["complete"] is True and witness["whole_goal_complete"] is False
            and witness["owned_directory_removed"] is True
            and digest(witness["runner_source"].encode()) == witness["runner_sha256"],
            "Complete locked-source pre-edit witness required")
    kinds = {"adapter", "logging", "configuration"}
    original_results = {(row["kind"], row["optimized"], row["regression"]): row
                        for row in witness["results"]}
    require(len(witness["results"]) == len(original_results) == 12
            and set(original_results) == {(kind, mode, regression) for kind in kinds
                                         for mode in (False, True) for regression in (False, True)},
            "Complete ordinary/optimized pre-edit result matrix required")
    for (kind, mode, regression), row in original_results.items():
        failure = regression and not mode
        require(row["exit_code"] == int(failure)
                and row["trace"]["exit_code"] == int(failure)
                and row["trace"]["optimize"] == int(mode)
                and (row["trace"]["error"]["type"] == "AssertionError" if failure
                     else row["trace"]["error"] is None),
                "Pre-edit regression was not the removed assertion")
    originals = {row["path"]: row for row in witness["sources"]}
    rows = manifest["rows"]
    require(len(originals) == len(rows) == 3 and {row["kind"] for row in rows} == kinds
            and sum(row["assertions"] for row in rows) == 16
            and all(not row["adaptations"] for row in rows),
            "Three assertion-only source transitions required")
    for row in rows:
        check_row(row, originals[row["historical_path"]])
    for path, wanted in manifest["dependencies"].items():
        require(digest((ROOT / path).read_bytes()) == wanted, "Imported control dependency changed")
    registered = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    tests = {row["name"]: row for row in registered}
    check_bindings(manifest["bindings"], tests, args.build_dir)
    source_controls = []
    first = rows[0]
    rejected("wrong_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), source_controls)
    rejected("changed_active_source", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), source_controls)
    changed = copy.deepcopy(tests)
    binding = manifest["bindings"][0]
    changed[binding["test"]]["command"][1] = str(ROOT / first["historical_path"])
    rejected("historical_registration", lambda: check_bindings(
        manifest["bindings"], changed, args.build_dir), source_controls)
    changed = copy.deepcopy(tests)
    binding = next(row for row in manifest["bindings"] if row["kind"] == "configuration")
    changed[binding["test"]]["properties"] = []
    rejected("missing_configuration_fixture", lambda: check_bindings(
        manifest["bindings"], changed, args.build_dir), source_controls)
    runner_source = witness["runner_source"]
    require(runner_source.count(BUILD_ARGUMENT) == 1, "Ambiguous pre-edit runner build argument")
    runner_source = runner_source.replace(BUILD_ARGUMENT, "sys.argv[6]", 1)
    results = []
    with tempfile.TemporaryDirectory(prefix="teaql_boundary_controls_506_") as owned:
        runner = Path(owned) / "runner.py"
        runner.write_text(runner_source)
        for row in rows:
            kind = row["kind"]
            for optimized in (False, True):
                for regression in (False, True):
                    trace_path = Path(owned) / (kind + str(optimized) + str(regression) + ".json")
                    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
                    environment.pop("PYTHONOPTIMIZE", None)
                    environment.pop("PYTHONPATH", None)
                    command = [sys.executable, "-B", *(["-O"] if optimized else []), str(runner),
                               str(ROOT), kind, str(ROOT / row["current_path"]), str(trace_path),
                               "regression" if regression else "valid", str(args.build_dir.resolve())]
                    result = subprocess.run(command, cwd=ROOT, env=environment,
                                            capture_output=True, text=True, timeout=180)
                    require(trace_path.is_file(), "Control process did not finish its observer")
                    trace = json.loads(trace_path.read_text())
                    expected = original_results[(kind, False, regression)]
                    require(result.returncode == trace["exit_code"] == int(regression)
                            and trace["optimize"] == int(optimized)
                            and trace["calls"] == expected["trace"]["calls"]
                            and result.stdout == expected["stdout"],
                            "Control checks or imported calls disappeared under optimization: "
                            + kind + result.stderr[-1000:])
                    if regression:
                        require(trace["error"] is not None
                                and trace["error"]["type"] == "AssertionError"
                                and trace["error"]["message"] == expected["trace"]["error"]["message"],
                                "Controlled dependency regression was accepted or failed elsewhere")
                    else:
                        require(trace["error"] is None, "Valid dependency behavior rejected")
                    results.append({"kind": kind, "optimized": optimized, "regression": regression,
                                    "exit_code": result.returncode, "stdout": result.stdout,
                                    "stderr": result.stderr, "trace": trace})
        removed = sum(path.stat().st_size for path in Path(owned).rglob("*") if path.is_file())
    require(not Path(owned).exists(), "Owned control observer outputs remain")
    if args.record:
        record = {"complete": True, "whole_goal_complete": False,
                  "manifest_sha256": digest(MANIFEST.read_bytes()), "results": results,
                  "source_controls": source_controls, "runner_source": runner_source,
                  "runner_sha256": digest(runner_source.encode()),
                  "owned_directory_removed": True, "owned_bytes_removed": removed}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Three control gates: sixteen unconditional checks, twelve actual valid/regression "
          "processes and four source/registration controls pass")


if __name__ == "__main__":
    main()
