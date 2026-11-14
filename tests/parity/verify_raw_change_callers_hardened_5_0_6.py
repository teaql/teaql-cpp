#!/usr/bin/env python3
"""Verify four control callers actually corrupt decoded source observations."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import check_row, digest, require


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/parity/raw_change_callers_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "ddf107eaf1083c23ab346d15209c14aa05fe2845e192ffb47194538fc3eec8a5"
WITNESS = ROOT / "docs/parity/raw_change_callers_preedit_5_0_6.json.gz"
ACTIVE_HELPER = "verify_expression_cli_source_controls_hardened_5_0_6"
RUNNER = '''import copy,importlib,json,sys
from pathlib import Path
root=Path(sys.argv[1]);sys.path.insert(0,str(root/'scripts'));sys.path.insert(0,str(root/'tests/parity'))
entry=importlib.import_module(sys.argv[2])
from compare_values import same,json_lines
if entry.raw_change.__module__ != 'verify_expression_cli_source_controls_hardened_5_0_6':
    raise RuntimeError('The active caller did not import the unconditional helper')
if sys.argv[3]=='no_string':
    value={'results':[{'step':0}],'steps':[{'stdout':'{"flag":true,"count":1}\\n'}]}
    before=copy.deepcopy(value)
    try:
        entry.raw_change(value,'value')
    except AssertionError:
        if not same(value,before):
            raise RuntimeError('A failed mutation changed its input')
        print(json.dumps({'unmutatable_rejected':True,'optimize':sys.flags.optimize}))
    else:
        raise RuntimeError('A record without an observed string was accepted')
else:
    validator=entry.Validator(root,entry.read(entry.REGISTRY)).load()
    validator.validate_source(validator.source)
    changed=copy.deepcopy(validator.source)
    entry.raw_change(changed,'value')
    step=changed['results'][0]['step']
    value_changed=not same(json_lines(changed['steps'][step]['stdout']),
                           json_lines(validator.source['steps'][step]['stdout']))
    if not value_changed:
        raise RuntimeError('Source corruption changed formatting without changing a value')
    try:
        validator.validate_source(changed)
    except (RuntimeError,ValueError,KeyError,TypeError,IndexError,StopIteration):
        rejected=True
    else:
        rejected=False
    if not rejected:
        raise RuntimeError('Changed raw observations were accepted')
    print(json.dumps({'value_changed':True,'rejected':True,'optimize':sys.flags.optimize}))
'''


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_bindings(bindings, registered, build):
    require(len(bindings) == 4, "Four raw-change control registrations")
    for binding in bindings:
        require(binding["test"] in registered, "Missing source control caller")
        command = registered[binding["test"]]["command"]
        current = ["${PYTHON}", *[value.replace(str(build.resolve()), "${BINARY_DIR}")
                                  .replace(str(ROOT), "${SOURCE_DIR}") for value in command[1:]]]
        require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                and current == binding["command"], "Raw-change caller registration changed")


def rejected(label, action, outcomes):
    try:
        action()
    except AssertionError:
        outcomes.append(label)
    else:
        raise AssertionError("Corrupt caller source accepted: " + label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve existing caller evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Caller source transition changed")
    manifest, witness = read(MANIFEST), read(WITNESS)
    require(manifest["complete"] is True and manifest["whole_goal_complete"] is False
            and manifest["scripts"] == 4 and manifest["assertions"] == 6
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a"
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and witness["complete"] is True and witness["whole_goal_complete"] is False
            and witness["owned_directory_removed"] is True
            and hashlib.sha256(witness["runner_source"].encode()).hexdigest() == witness["runner_sha256"],
            "Complete bounded pre-edit source witness")
    require(len(witness["results"]) == 8
            and {(r["family"], r["optimized"]) for r in witness["results"]}
            == {(name, mode) for name in witness["families"] for mode in (False, True)}
            and all(row["exit_code"] == 0
                    and row["outcome"]["value_changed"] is (not row["optimized"])
                    and row["outcome"]["rejected"] is (not row["optimized"])
                    and row["outcome"]["optimize"] == int(row["optimized"])
                    for row in witness["results"]), "Four actual removed-mutation counterexamples")
    originals = {row["path"]: row for row in witness["source_rows"]}
    require(len(originals) == 5 and len(manifest["rows"]) == 4
            and sum(row["assertions"] for row in manifest["rows"]) == 6
            and sum(len(row["adaptations"]) for row in manifest["rows"]) == 4,
            "Four caller rewrites and one retained historical helper")
    for row in manifest["rows"]:
        check_row(row, originals[row["historical_path"]])
    historical_helper = originals["tests/parity/verify_expression_cli_source_controls_5_0_6.py"]
    require(digest((ROOT / historical_helper["path"]).read_bytes()) == historical_helper["sha256"],
            "Historical shared helper changed")
    for path, wanted in manifest["dependencies"].items():
        require(digest((ROOT / path).read_bytes()) == wanted, "Caller helper or retained source changed")
    helper_manifest = read(ROOT / "docs/parity/generated_cli_hardened_transition_5_0_6.json.gz")
    helper_row = next(row for row in helper_manifest["rows"]
                      if row["historical_path"] == historical_helper["path"])
    check_row(helper_row, historical_helper)
    tests = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    registered = {row["name"]: row for row in tests}
    check_bindings(manifest["bindings"], registered, args.build_dir)
    source_controls = []
    first = manifest["rows"][0]
    rejected("changed_historical_hash", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), source_controls)
    rejected("changed_active_source", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), source_controls)
    rejected("old_helper_import", lambda: check_row(
        dict(first, adaptations=[]), originals[first["historical_path"]]), source_controls)
    altered = copy.deepcopy(registered)
    binding = manifest["bindings"][0]
    historical = next(row["historical_path"] for row in manifest["rows"]
                      if "${SOURCE_DIR}/" + row["current_path"] == binding["command"][1])
    altered[binding["test"]]["command"][1] = str(ROOT / historical)
    rejected("old_control_registration", lambda: check_bindings(
        manifest["bindings"], altered, args.build_dir), source_controls)
    results = []
    with tempfile.TemporaryDirectory(prefix="teaql_raw_change_callers_controls_506_") as owned:
        runner = Path(owned) / "runner.py"
        runner.write_text(RUNNER)
        for row in manifest["rows"]:
            name = Path(row["current_path"]).stem
            for optimized in (False, True):
                for mode in ("value", "no_string"):
                    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
                    environment.pop("PYTHONOPTIMIZE", None)
                    environment.pop("PYTHONPATH", None)
                    command = [sys.executable, "-B", *(["-O"] if optimized else []),
                               str(runner), str(ROOT), name, mode]
                    result = subprocess.run(command, cwd=ROOT, env=environment,
                                            capture_output=True, text=True, timeout=180)
                    require(result.returncode == 0, "Active caller mutation failed: " + result.stderr[-1500:])
                    outcome = json.loads(result.stdout)
                    require(outcome["optimize"] == int(optimized)
                            and (outcome == {"value_changed": True, "rejected": True,
                                             "optimize": int(optimized)} if mode == "value"
                                 else outcome == {"unmutatable_rejected": True, "optimize": int(optimized)}),
                            "Incorrect decoded-value mutation outcome")
                    results.append({"caller": name, "optimized": optimized, "mode": mode,
                                    "exit_code": result.returncode, "outcome": outcome})
        removed = runner.stat().st_size
    require(not Path(owned).exists(), "Owned caller observer source remains")
    if args.record:
        record = {"complete": True, "whole_goal_complete": False,
                  "manifest_sha256": digest(MANIFEST.read_bytes()), "source_controls": source_controls,
                  "results": results, "runner_source": RUNNER,
                  "runner_sha256": hashlib.sha256(RUNNER.encode()).hexdigest(),
                  "owned_directory_removed": True, "owned_bytes_removed": removed}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Four control callers: six unconditional checks, four import transitions, "
          "normal/optimized decoded-value corruption and unmutatable-input checks pass")


if __name__ == "__main__":
    main()
