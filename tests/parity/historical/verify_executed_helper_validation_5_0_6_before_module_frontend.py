#!/usr/bin/env python3
"""Pin executed recipe/privacy checks and their qualified caller registrations."""

import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from compare_values import same
from verify_generated_driver_hardened_sources_5_0_6 import check_row, digest, require


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/parity/executed_helper_validation_registered_5_0_6.json.gz"
MANIFEST_SHA256 = "f1713f6493707c428efaa689332703f2800140d737addbe83265a0d16cfcd017"


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_bindings(bindings, tests, build):
    require(len(bindings) == 3, "Three qualified helper caller registrations")
    for binding in bindings:
        require(binding["test"] in tests, "Missing helper caller")
        test = tests[binding["test"]]
        command = test["command"]
        normalized = ["${PYTHON}", *[
            part.replace(str(build.resolve()), "${BINARY_DIR}")
            .replace(str(ROOT), "${SOURCE_DIR}") for part in command[1:]]]
        require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                and normalized == binding["command"]
                and test["properties"] == binding["properties"],
                "Qualified helper command or required fixture changed")


def rejected(label, action, results):
    try:
        action()
    except AssertionError:
        results.append(label)
    else:
        raise AssertionError("Corrupted executed-helper evidence accepted: " + label)


def environment():
    value = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
    value.pop("PYTHONOPTIMIZE", None)
    value.pop("PYTHONPATH", None)
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve executed-helper evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Executed-helper manifest changed")
    manifest = read(MANIFEST)
    require(manifest["complete"] is True and manifest["whole_goal_complete"] is False
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a",
            "Locked helper baseline and bounded scope required")
    for reference in manifest["references"].values():
        require(digest((ROOT / reference["path"]).read_bytes()) == reference["sha256"],
                "Executed-helper source or retained controls changed")
    candidate = read(ROOT / manifest["references"]["candidate"]["path"])
    witness = read(ROOT / manifest["references"]["witness"]["path"])
    controls = read(ROOT / manifest["references"]["controls"]["path"])
    cli = read(ROOT / manifest["references"]["cli"]["path"])
    require(candidate["complete"] is True and candidate["installed"] is False
            and candidate["scripts"] == 5 and candidate["assertions"] == 14
            and candidate["adaptations"] == 5 and len(candidate["rows"]) == 5
            and witness["complete"] is True and witness["runtime_execution"] is False
            and witness["owned_directory_removed"] is True
            and digest(witness["runner_source"].encode()) == witness["runner_sha256"],
            "Complete pre-activation source witness required")
    originals = {row["path"]: row for row in candidate["source_rows"]}
    require(len(originals) == 5 and sum(row["assertions"] for row in candidate["rows"]) == 14
            and sum(len(row["adaptations"]) for row in candidate["rows"]) == 5,
            "Complete helper and caller source inventory required")
    for row in candidate["rows"]:
        check_row(row, originals[row["historical_path"]])
    historical = {(row["kind"], row["case"], row["optimized"]): row for row in witness["results"]}
    require(len(witness["results"]) == len(historical) == 24
            and len(controls["results"]) == 24 and controls["complete"] is True
            and controls["installed"] is False and controls["owned_directory_removed"] is True
            and digest(controls["runner_source"].encode()) == controls["runner_sha256"]
            and digest(controls["parity_source"].encode()) == controls["parity_sha256"],
            "Complete actual helper controls and valid-data observers required")
    for (kind, case, mode), row in historical.items():
        expected = 0 if case.startswith("valid") or mode else 1
        require(row["exit_code"] == row["trace"]["exit_code"] == expected
                and row["trace"]["optimize"] == int(mode)
                and (row["trace"]["error"] is None if expected == 0
                     else row["trace"]["error"]["type"] == "AssertionError")
                and (kind, case, not mode) in historical,
                "Pre-edit observation did not expose the helper assertion boundary")
    require(cli["complete"] is True and cli["installed"] is False
            and cli["owned_directory_removed"] is True and len(cli["runs"]) == 6
            and {(row["name"], row["optimized"]) for row in cli["runs"]}
            == {(name, mode) for name in ("source", "controls", "privacy") for mode in (False, True)}
            and all(row["exit_code"] == 0 for row in cli["runs"]),
            "Six retained source/control/native caller executions required")
    entries = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    tests = {row["name"]: row for row in entries}
    check_bindings(manifest["bindings"], tests, args.build_dir)
    require(manifest["guard_test"] in tests
            and tests[manifest["guard_test"]]["command"][1:]
            == [str(ROOT / manifest["guard_script"]), "--build-dir", str(args.build_dir.resolve())]
            and {row["name"]: row["value"] for row in tests[manifest["guard_test"]]["properties"]}
            .get("FIXTURES_SETUP") == [manifest["fixture"]],
            "Executed-helper guard fixture changed")
    source_controls = []
    first = candidate["rows"][0]
    rejected("wrong_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), source_controls)
    rejected("changed_active_helper", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), source_controls)
    parent = next(row for row in candidate["rows"] if row["assertions"] == 0)
    rejected("missing_caller_import_admission", lambda: check_row(
        dict(parent, adaptations=[]), originals[parent["historical_path"]]), source_controls)
    changed = copy.deepcopy(tests)
    binding = manifest["bindings"][0]
    changed[binding["test"]]["command"][1] = binding["historical_command"][1].replace("${SOURCE_DIR}", str(ROOT))
    rejected("historical_caller_registration", lambda: check_bindings(
        manifest["bindings"], changed, args.build_dir), source_controls)
    changed = copy.deepcopy(tests)
    changed[binding["test"]]["properties"] = [
        row for row in changed[binding["test"]]["properties"] if row["name"] != "FIXTURES_REQUIRED"]
    rejected("missing_source_guard_fixture", lambda: check_bindings(
        manifest["bindings"], changed, args.build_dir), source_controls)
    results, valid_matrix = [], []
    with tempfile.TemporaryDirectory(prefix="teaql_executed_helper_registered_506_") as owned:
        directory = Path(owned)
        runner, parity = directory / "runner.py", directory / "parity.py"
        runner.write_text(controls["runner_source"])
        parity.write_text(controls["parity_source"])
        for index, ((kind, case, mode), previous) in enumerate(sorted(historical.items())):
            trace_path = directory / ("trace_" + str(index) + ".json")
            command = [sys.executable, "-B", *(["-O"] if mode else []), str(runner),
                       str(ROOT), kind, case, str(trace_path)]
            result = subprocess.run(command, cwd=ROOT, env=environment(),
                                    capture_output=True, text=True, timeout=60)
            require(trace_path.is_file(), "Imported helper observer did not finish")
            trace = json.loads(trace_path.read_text())
            valid = case.startswith("valid")
            normal = historical[(kind, case, False)]["trace"]
            require(result.returncode == trace["exit_code"] == int(not valid)
                    and trace["optimize"] == int(mode)
                    and (same(trace["outcome"], normal["outcome"]) and trace["error"] is None if valid
                         else trace["error"] is not None
                         and trace["error"]["type"] == "AssertionError"
                         and trace["error"]["message"] == normal["error"]["message"]),
                    "Invalid helper data accepted or valid helper behavior changed")
            results.append({"kind": kind, "case": case, "optimized": mode,
                            "exit_code": result.returncode, "trace": trace})
        for mode in (False, True):
            result = subprocess.run([sys.executable, "-B", *(["-O"] if mode else []),
                                     str(parity), str(ROOT)], cwd=ROOT, env=environment(),
                                    capture_output=True, text=True, timeout=120)
            require(result.returncode == 0, "Complete helper input parity failed: " + result.stderr[-1000:])
            outcome = json.loads(result.stdout)
            require(outcome["recipes"] == 7 and outcome["recipe_inputs"] == 6281
                    and outcome["privacy_inputs"] == 5486 and outcome["optimize"] == int(mode)
                    and outcome["privacy_summary"] == controls["valid_matrix"][int(mode)]["outcome"]["privacy_summary"],
                    "Incomplete input/proof/privacy summary comparison")
            valid_matrix.append({"optimized": mode, "exit_code": 0, "outcome": outcome})
        removed = sum(path.stat().st_size for path in directory.rglob("*") if path.is_file())
    require(not directory.exists(), "Owned executed-helper observer outputs remain")
    if args.record:
        record = {"complete": True, "whole_goal_complete": False, "manifest_sha256": digest(MANIFEST.read_bytes()),
                  "results": results, "valid_matrix": valid_matrix, "source_controls": source_controls,
                  "owned_directory_removed": True, "owned_bytes_removed": removed}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Executed helpers: fourteen unconditional checks; 24 valid/invalid processes; "
          "6281 recipe inputs and 5486 privacy inputs unchanged in both modes; five source controls pass")


if __name__ == "__main__":
    main()
