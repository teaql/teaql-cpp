#!/usr/bin/env python3
"""Qualify generator checks reached through the two active source prerequisites."""

import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import check_row, digest, require
from frontend_bindings_5_0_6 import fixture_name, test_name


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/parity/generator_verifier_child_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "227570481ab112f10e96f45e0bd493d472f763889533a458ffb80143f25e0cd6"
WITNESS = ROOT / "docs/parity/generator_verifier_child_preedit_5_0_6.json.gz"
BINDINGS = {
    "parity_runtime_configuration_source_contracts": (
        "scripts/verify_runtime_configuration_source_hardened_5_0_6.py", "runtime_configuration_source"),
    "parity_legacy_observation_identity_contracts": (
        "scripts/verify_legacy_observation_identity_hardened_5_0_6.py", "legacy_observation_source"),
}
RUNNER = '''import json,runpy,subprocess,sys,traceback
from pathlib import Path
from unittest.mock import patch
root=Path(sys.argv[1]);entry=sys.argv[2];case=sys.argv[3];out=Path(sys.argv[4])
names={'child':'scripts/verify_generator_parity_hardened_5_0_6.py',
       'runtime_parent':'scripts/verify_runtime_configuration_source_hardened_5_0_6.py',
       'legacy_parent':'scripts/verify_legacy_observation_identity_hardened_5_0_6.py'}
script=root/names[entry];sys.path.insert(0,str(root/'scripts'));sys.argv=[str(script)]
trace={'entry':entry,'case':case,'optimize':sys.flags.optimize,'calls':[],'error':None,'exit_code':0}
target=root/'tests/generator/parity/query_alias_contracts/rust/PROVENANCE.json'
original_text=Path.read_text;value=json.loads(original_text(target))
if case=='hand_edited':value['hand_edited']=True
elif case=='wrong_model_hash':value['model_sha256']='0'*64
elif case!='valid':raise RuntimeError('Unknown generator child case')
def text(path,*args,**kwargs):
    return json.dumps(value) if path.resolve()==target.resolve() else original_text(path,*args,**kwargs)
original_run=subprocess.run
def child(command,*args,**kwargs):
    relevant=len(command)>1 and str(command[1]) in (str(root/names['child']),str(root/'scripts/verify_upstream.py'))
    if not relevant:return original_run(command,*args,**kwargs)
    observed={'command':list(map(str,command))};actual=list(command)
    if str(command[1])==str(root/names['child']):
        actual=[sys.executable,'-B',*(['-O'] if sys.flags.optimize else []),__file__,str(root),'child',case,str(out.with_name('child_'+out.name))]
    observed['actual_command']=list(map(str,actual));trace['calls'].append(observed)
    try:
        result=original_run(actual,*args,**kwargs);observed['exit_code']=result.returncode;return result
    except subprocess.CalledProcessError as error:
        observed['exit_code']=error.returncode;raise
try:
    if entry=='child':
        with patch.object(Path,'read_text',text):runpy.run_path(str(script),run_name='__main__')
    else:
        with patch.object(subprocess,'run',child):runpy.run_path(str(script),run_name='__main__')
except BaseException as error:
    trace['exit_code']=1;trace['error']={'type':type(error).__name__,'message':str(error),'frames':[{'path':f.filename,'line':f.lineno,'name':f.name} for f in traceback.extract_tb(error.__traceback__)]}
    out.write_text(json.dumps(trace,sort_keys=True));raise
else:out.write_text(json.dumps(trace,sort_keys=True))
'''


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def check_bindings(tests, frontend="native"):
    for name, (script, fixture) in BINDINGS.items():
        name = test_name(name, frontend)
        require(name in tests, "Missing generator-child source prerequisite")
        test = tests[name]
        props = {row["name"]: row["value"] for row in test["properties"]}
        require(Path(test["command"][0]).resolve() == Path(sys.executable).resolve()
                and test["command"][1:] == [str(ROOT / script)]
                and props.get("FIXTURES_SETUP") == [fixture_name(fixture, frontend)]
                and fixture_name("generator_child_checks", frontend)
                in props.get("FIXTURES_REQUIRED", [])
                and props.get("RUN_SERIAL") is True and props.get("TIMEOUT") == 180,
                "Generator child parent command or source/guard fixture changed")
    guard = test_name("parity_generator_child_checks", frontend)
    require(guard in tests
            and {row["name"]: row["value"] for row in tests[guard]["properties"]}
            .get("FIXTURES_SETUP") == [fixture_name("generator_child_checks", frontend)],
            "Generator child source/control guard is not registered")


def rejected(label, action, results):
    try:
        action()
    except AssertionError:
        results.append(label)
    else:
        raise AssertionError("Generator child source corruption accepted: " + label)


def execute(command, environment):
    process = subprocess.Popen(command, cwd=ROOT, env=environment,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True, start_new_session=True)
    try:
        stdout, stderr = process.communicate(timeout=180)
    except BaseException:
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
        try:
            process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
        raise
    return {"command": command, "exit_code": process.returncode,
            "stdout": stdout, "stderr": stderr}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--frontend", choices=("native", "module"), default="native")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve generator child evidence")
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Generator child transition changed")
    manifest, witness = read(MANIFEST), read(WITNESS)
    require(manifest["complete"] is True and manifest["installed"] is False
            and manifest["whole_goal_complete"] is False and manifest["scripts"] == 3
            and manifest["assertions"] == 21 and manifest["adaptations"] == 2
            and len(manifest["rows"]) == len(manifest["source_rows"]) == 3
            and manifest["runtime_commit"] == witness["runtime_commit"]
            == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == witness["generator_commit"]
            == "45e1572c4956ca963410d8ea436ead3c8857087a"
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and witness["complete"] is True and witness["owned_directory_removed"] is True
            and digest(witness["runner_source"].encode()) == witness["runner_sha256"],
            "Exact source transition and complete actual pre-edit witness required")
    originals = {row["path"]: row for row in manifest["source_rows"]}
    for row in manifest["rows"]:
        check_row(row, originals[row["historical_path"]])
    require(len(witness["results"]) == 6
            and {(row["case"], row["optimized"]) for row in witness["results"]}
            == {(case, mode) for case in ("valid", "hand_edited", "wrong_model_hash")
                for mode in (False, True)}
            and all(row["exit_code"] == (0 if row["case"] == "valid" or row["optimized"] else 1)
                    for row in witness["results"]), "Complete two-mode false-acceptance witness required")
    entries = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))["tests"]
    tests = {row["name"]: row for row in entries}
    check_bindings(tests, args.frontend)
    source_controls = []
    first, parent = manifest["rows"][0], manifest["rows"][1]
    rejected("wrong_historical_source", lambda: check_row(
        dict(first, before_sha256="0" * 64), originals[first["historical_path"]]), source_controls)
    rejected("changed_active_generator", lambda: check_row(
        first, originals[first["historical_path"]], first["after_source"] + "\n"), source_controls)
    rejected("missing_child_call_admission", lambda: check_row(
        dict(parent, adaptations=[]), originals[parent["historical_path"]]), source_controls)
    changed = copy.deepcopy(tests)
    runtime_parent = test_name("parity_runtime_configuration_source_contracts", args.frontend)
    changed[runtime_parent]["command"][1] = str(ROOT / parent["historical_path"])
    rejected("historical_parent_registration", lambda: check_bindings(changed, args.frontend), source_controls)
    changed = copy.deepcopy(tests)
    changed[runtime_parent]["properties"] = [
        row for row in changed[runtime_parent]["properties"]
        if row["name"] != "FIXTURES_REQUIRED"]
    rejected("missing_child_guard", lambda: check_bindings(changed, args.frontend), source_controls)
    results = []
    with tempfile.TemporaryDirectory(prefix="teaql_generator_child_checks_506_") as owned:
        directory = Path(owned)
        runner = directory / "runner.py"
        runner.write_text(RUNNER)
        for mode in (False, True):
            env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
            env.pop("PYTHONOPTIMIZE", None)
            env.pop("PYTHONPATH", None)
            if mode:
                env["PYTHONOPTIMIZE"] = "1"
            for entry in ("child", "runtime_parent", "legacy_parent"):
                for case in ("valid", "hand_edited", "wrong_model_hash"):
                    output = directory / (entry + "_" + case + "_" + str(mode) + ".json")
                    result = execute([sys.executable, "-B", *(["-O"] if mode else []),
                                      str(runner), str(ROOT), entry, case, str(output)], env)
                    require(output.is_file(), "Generator child observer did not finish")
                    trace = json.loads(output.read_text())
                    valid = case == "valid"
                    require(result["exit_code"] == trace["exit_code"] == int(not valid)
                            and trace["optimize"] == int(mode)
                            and (trace["error"] is None if valid else trace["error"]["type"]
                                 == ("AssertionError" if entry == "child" else "CalledProcessError")),
                            "Wrong generator child/parent acceptance outcome")
                    if entry != "child":
                        nested = directory / ("child_" + output.name)
                        require(nested.is_file() and len(trace["calls"]) == 2
                                and trace["calls"][0]["command"][1] == str(ROOT / "scripts/verify_upstream.py")
                                and trace["calls"][0]["exit_code"] == 0
                                and trace["calls"][1]["command"][1]
                                == str(ROOT / "scripts/verify_generator_parity_hardened_5_0_6.py")
                                and trace["calls"][1]["exit_code"] == int(not valid),
                                "Actual parent did not enforce the intended child result")
                        trace["child"] = json.loads(nested.read_text())
                        require(trace["child"]["optimize"] == int(mode)
                                and trace["child"]["exit_code"] == int(not valid),
                                "Parent/child optimization or rejection differs")
                    results.append(dict(result, entry=entry, case=case, optimized=mode, trace=trace))
        for entry in ("child", "runtime_parent", "legacy_parent"):
            valid = [row for row in results if row["entry"] == entry and row["case"] == "valid"]
            require(len(valid) == 2 and valid[0]["stdout"] == valid[1]["stdout"],
                    "Valid generator/parent output changed under optimization")
        removed = sum(path.stat().st_size for path in directory.rglob("*") if path.is_file())
    require(not directory.exists(), "Owned generator child outputs remain")
    if args.record:
        record = {"complete": True, "whole_goal_complete": False, "complete_sq02": False,
                  "manifest_sha256": MANIFEST_SHA256, "frontend": args.frontend, "results": results,
                  "source_controls": source_controls, "owned_directory_removed": True,
                  "owned_bytes_removed": removed}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Generator child: 21 unconditional checks; 18 valid/invalid child and actual-parent processes; "
          "five source/registration controls reject in both Python modes")


if __name__ == "__main__":
    main()
