#!/usr/bin/env python3
"""Qualify the remaining registered checks and their embedded observer programs."""

import argparse
import ast
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import (
    ReplaceAssert, check_row, digest, require,
)


ROOT = Path(__file__).resolve().parents[2]
WITNESS = ROOT / "docs/parity/registered_boundary_preedit_5_0_6.json.gz"
MANIFEST = ROOT / "docs/parity/registered_boundary_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "87235043235ebe3f31ad3048aa85afcf79555f8f7489eb571ab5732a13721641"


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_embedded(row):
    before, after = row["before_source"], row["after_source"]
    old_tree, new_tree = ast.parse(before), ast.parse(after)
    require(digest(before.encode()) == row["before_sha256"]
            and digest(after.encode()) == row["after_sha256"]
            and ast.literal_eval("(" + row["before_literal"] + ")") == before
            and ast.literal_eval("(" + row["after_literal"] + ")") == after,
            "Embedded observer source identity changed")
    require(sum(isinstance(node, ast.Assert) for node in ast.walk(old_tree)) == row["assertions"]
            and not any(isinstance(node, ast.Assert) for node in ast.walk(new_tree))
            and ast.dump(ReplaceAssert().visit(old_tree), include_attributes=False)
            == ast.dump(new_tree, include_attributes=False),
            "Embedded observer has changes beyond unconditional checks")
    require(not before.startswith("#!") or after.splitlines()[0] == before.splitlines()[0],
            "Executable observer lost its interpreter directive")


def check_bindings(bindings, tests, build_dir, expected=42):
    require(len(bindings) == expected, "Complete active boundary registrations required")
    for binding in bindings:
        require(binding["test"] in tests, "Missing boundary registration")
        test = tests[binding["test"]]
        command = test["command"]
        normalized = ["${PYTHON}", *[
            value.replace(str(build_dir.resolve()), "${BINARY_DIR}")
            .replace(str(ROOT), "${SOURCE_DIR}") for value in command[1:]]]
        require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                and normalized == binding["command"]
                and test["properties"] == binding["properties"],
                "Boundary script, command or fixture properties changed")


def rejected(label, action, outcomes):
    try:
        action()
    except AssertionError:
        outcomes.append(label)
    else:
        raise AssertionError("Accepted corrupted boundary transition: " + label)


def environment():
    result = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
    result.pop("PYTHONOPTIMIZE", None)
    result.pop("PYTHONPATH", None)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve boundary evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Boundary transition changed")
    manifest, witness = read(MANIFEST), read(WITNESS)
    require(manifest["complete"] is True and manifest["whole_goal_complete"] is False
            and manifest["scripts"] == 42 and manifest["assertions"] == 117
            and manifest["embedded_scripts"] == 9 and manifest["embedded_assertions"] == 10
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a"
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and witness["complete"] is True and witness["whole_goal_complete"] is False
            and witness["runtime_execution"] is False
            and witness["owned_directory_removed"] is True
            and digest(witness["runner_source"].encode()) == witness["runner_sha256"],
            "Complete locked-source boundary witness required")
    originals = {row["path"]: row for row in witness["sources"]}
    rows = manifest["rows"]
    require(len(rows) == len(originals) == 42
            and {row["historical_path"] for row in rows} == set(originals)
            and sum(row["assertions"] for row in rows) == 117
            and sum(len(row["adaptations"]) for row in rows) == 12,
            "Remaining registered transition inventory changed")
    for row in rows:
        check_row(row, originals[row["historical_path"]])
    embedded = manifest["embedded_rows"]
    require(len(embedded) == 9 and sum(row["assertions"] for row in embedded) == 10,
            "Embedded transition inventory changed")
    for row in embedded:
        check_embedded(row)
        parent = next(parent for parent in rows if parent["historical_path"] == row["historical_path"])
        require({"before": row["before_literal"], "after": row["after_literal"]}
                in parent["adaptations"], "Embedded transition is not the installed literal")
    for path, wanted in manifest["dependencies"].items():
        require(digest((ROOT / path).read_bytes()) == wanted, "Local imported dependency changed")
    support = manifest["support"]
    for item in support.values():
        require(digest((ROOT / item["path"]).read_bytes()) == item["sha256"],
                "Qualified helper or width-binding source changed")
    helper_transition = read(ROOT / support["helper_transition"]["path"])
    helper_witness = read(ROOT / support["helper_witness"]["path"])
    helper_originals = {row["path"]: row for row in helper_witness["source_rows"]}
    require(helper_transition["complete"] is True
            and helper_transition["whole_goal_complete"] is False
            and helper_transition["assertions"] == manifest["imported_helper_assertions"] == 21
            and helper_transition["witness_sha256"] == support["helper_witness"]["sha256"]
            and len(helper_transition["rows"]) == 2
            and helper_witness["complete"] is True
            and helper_witness["owned_directory_removed"] is True
            and digest(helper_witness["runner_source"].encode()) == helper_witness["runner_sha256"],
            "Complete imported-helper transition and original execution required")
    for row in helper_transition["rows"]:
        check_row(row, helper_originals[row["historical_path"]])
    helper_controls = read(ROOT / support["helper_controls"]["path"])
    width_controls = read(ROOT / support["width_controls"]["path"])
    require(helper_controls["complete"] is True and len(helper_controls["results"]) == 8
            and helper_controls["manifest_sha256"] == support["helper_transition"]["sha256"]
            and digest(helper_controls["runner_source"].encode()) == helper_controls["runner_sha256"]
            and width_controls["complete"] is True and len(width_controls["runs"]) == 2
            and width_controls["wrapper_sha256"] == support["width_wrapper"]["sha256"]
            and digest(width_controls["runner_source"].encode()) == width_controls["runner_sha256"],
            "Complete helper and width-binding control sources required")
    tests = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    registered = {row["name"]: row for row in tests}
    check_bindings(manifest["bindings"], registered, args.build_dir)
    check_bindings(manifest["support_bindings"], registered, args.build_dir, expected=5)
    historical = {(row["path"], row["kind"], row["case"], row["optimized"]): row
                  for row in witness["results"]}
    require(len(witness["results"]) == len(historical) == 48,
            "Complete pre-edit process matrix required")
    for key, row in historical.items():
        valid = row["case"] == "valid"
        expected = 0 if valid or row["optimized"] else 1
        require(row["exit_code"] == row["trace"]["exit_code"] == expected
                and row["trace"]["optimize"] == int(row["optimized"])
                and (row["trace"]["error"] is None if expected == 0
                     else row["trace"]["error"]["type"] == "AssertionError"),
                "Pre-edit result did not expose an assertion boundary")
        require((key[0], key[1], key[2], not key[3]) in historical,
                "Pre-edit process lacks the other optimization mode")
    controls = []
    first = rows[0]
    rejected("changed_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), controls)
    rejected("changed_active_source", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), controls)
    rejected("embedded_source_substitution", lambda: check_embedded(
        dict(embedded[0], after_source=embedded[0]["before_source"])), controls)
    changed = copy.deepcopy(registered)
    binding = manifest["bindings"][0]
    changed[binding["test"]]["command"][1] = str(ROOT / first["historical_path"])
    rejected("historical_script_registration", lambda: check_bindings(
        manifest["bindings"], changed, args.build_dir), controls)
    helper = helper_transition["rows"][0]
    rejected("wrong_helper_check_count", lambda: check_row(
        dict(helper, assertions=20), helper_originals[helper["historical_path"]]), controls)
    dynamic = next(row for row in rows if "verify_dynamic_json_cli_admission_" in row["current_path"])
    statement = "    path.write_bytes(gzip.compress(json.dumps(data).encode(),mtime=0))\n"
    require(dynamic["assert_only_source"].count(statement) == 1, "Compound statement control changed")
    rejected("lost_neighboring_statement", lambda: check_row(
        dict(dynamic, assert_only_source=dynamic["assert_only_source"].replace(statement, "", 1)),
        originals[dynamic["historical_path"]]), controls)
    results, retained_files, helper_results, width_results, launches = [], [], [], [], []
    with tempfile.TemporaryDirectory(prefix="teaql_registered_boundary_controls_506_") as owned:
        directory = Path(owned)
        runner = directory / "runner.py"
        runner.write_text(witness["runner_source"])
        current = {row["historical_path"]: row["current_path"] for row in rows}
        observers = {row["historical_path"]: row["after_source"] for row in embedded}
        for index, (key, before) in enumerate(sorted(historical.items())):
            path, kind, case, optimized = key
            task = directory / str(index)
            task.mkdir()
            fixture = {"kind": kind, "path": current[path], "case": case}
            if kind == "embedded":
                fixture["source"] = observers[path]
            fixture_path, trace_path = task / "fixture.json", task / "trace.json"
            fixture_path.write_text(json.dumps(fixture))
            command = [sys.executable, "-B", *(["-O"] if optimized else []), str(runner),
                       str(ROOT), str(fixture_path), str(trace_path)]
            result = subprocess.run(command, cwd=ROOT, env=environment(),
                                    capture_output=True, text=True, timeout=60)
            require(trace_path.is_file(), "Boundary observer did not finish")
            trace = json.loads(trace_path.read_text())
            valid = case == "valid"
            expected = historical[(path, kind, case, False)]["trace"]
            require(result.returncode == trace["exit_code"] == int(not valid)
                    and trace["optimize"] == int(optimized)
                    and trace["stdout"] == expected["stdout"]
                    and trace["outcome"] == expected["outcome"]
                    and (trace["error"] is None if valid
                         else trace["error"] is not None
                         and trace["error"]["type"] == "AssertionError"
                         and trace["error"]["message"] == expected["error"]["message"]),
                    "Invalid input or nonzero replay result was accepted under optimization")
            results.append({"path": path, "kind": kind, "case": case,
                            "optimized": optimized, "exit_code": result.returncode, "trace": trace})
        resource = next(row for row in embedded if "resource_lifetime_admission" in row["historical_path"])
        require(resource["before_source"].startswith("\n")
                and resource["after_source"].startswith("\n"),
                "Resource observer must retain the interpreter-line terminator")
        for optimized in (False, True):
            for valid in (False, True):
                task = directory / ("resource_launch_" + str(optimized) + str(valid))
                task.mkdir()
                observer = task / "observer.py"
                observer.write_text("#!" + sys.executable + resource["after_source"])
                observer.chmod(0o700)
                observer.with_suffix(".json").write_text(json.dumps({
                    "inputs": [{"request": 1}], "outputs": [{"value": 1}], "framing": None}))
                env = environment()
                if optimized:
                    env["PYTHONOPTIMIZE"] = "1"
                result = subprocess.run([str(observer)], cwd=task, env=env,
                                        input='{"request":1}\n' if valid else '{"wrong":1}\n',
                                        capture_output=True, text=True, timeout=60)
                require(result.returncode == int(not valid)
                        and ([json.loads(line) for line in result.stdout.splitlines()] == [{"value": 1}]
                             if valid else "AssertionError" in result.stderr),
                        "Constructed resource observer did not execute or accepted wrong inputs")
                launches.append({"optimized": optimized, "valid": valid,
                                 "exit_code": result.returncode, "stdout": result.stdout})
        for label, receipt in (("helper", helper_controls), ("width", width_controls)):
            probe = directory / (label + "_runner.py")
            probe.write_text(receipt["runner_source"])
            cases = ("valid", "missing_pin", "wrong_hash", "unknown_version") if label == "helper" else (None,)
            for optimized in (False, True):
                for case in cases:
                    command = [sys.executable, "-B", *(["-O"] if optimized else []),
                               str(probe), str(ROOT), *([case] if case is not None else [])]
                    result = subprocess.run(command, cwd=ROOT, env=environment(),
                                            capture_output=True, text=True, timeout=60)
                    expected = int(label == "helper" and case != "valid")
                    require(result.returncode == expected
                            and (not expected or "AssertionError" in result.stderr),
                            "Imported helper or width-binding control failed")
                    if label == "helper":
                        helper_results.append({"case": case, "optimized": optimized,
                                               "exit_code": result.returncode})
                    else:
                        outcome = json.loads(result.stdout)
                        require(outcome["optimize"] == int(optimized) and len(outcome["results"]) == 9,
                                "Incomplete current width-binding controls")
                        width_results.append(outcome)
        for index, binding in enumerate(manifest["bindings"]):
            for optimized in (False, True):
                retained = directory / ("retained_" + str(index) + str(optimized))
                retained.write_bytes(b"Retained source evidence must not be overwritten.\n")
                wanted = digest(retained.read_bytes())
                command = [sys.executable, "-B", *(["-O"] if optimized else []),
                           *registered[binding["test"]]["command"][1:], "--record", str(retained)]
                result = subprocess.run(command, cwd=ROOT, env=environment(),
                                        capture_output=True, text=True, timeout=60)
                require(result.returncode != 0 and digest(retained.read_bytes()) == wanted
                        and ("AssertionError" in result.stderr or "error:" in result.stderr),
                        "Existing evidence was overwritten or accepted: " + binding["test"])
                retained_files.append({"test": binding["test"], "optimized": optimized,
                                       "exit_code": result.returncode, "unchanged_sha256": wanted})
        removed = sum(path.stat().st_size for path in directory.rglob("*") if path.is_file())
    require(not directory.exists(), "Owned boundary sources or traces remain")
    if args.record:
        record = {"complete": True, "whole_goal_complete": False,
                  "manifest_sha256": digest(MANIFEST.read_bytes()), "source_controls": controls,
                  "results": results, "retained_evidence_controls": retained_files,
                  "resource_launch_controls": launches, "helper_results": helper_results,
                  "width_results": width_results,
                  "runner_source": witness["runner_source"], "runner_sha256": witness["runner_sha256"],
                  "owned_directory_removed": True, "owned_bytes_removed": removed}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Registered boundaries: 117 direct, 10 embedded and 21 imported checks; "
          "48 body/replay, 4 executable-launch, 8 helper, 18 width-binding, "
          "84 retained-evidence and 6 source controls pass")


if __name__ == "__main__":
    main()
