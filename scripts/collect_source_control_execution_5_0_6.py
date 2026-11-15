#!/usr/bin/env python3
"""Observe the fixed source/control/invariant CTest cohort in both Python modes."""

import argparse
from collections import Counter
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import signal
import subprocess
import sys
import tempfile
import time
import traceback
import xml.etree.ElementTree as ET

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references


ROOT = Path(__file__).resolve().parents[1]
INVENTORY = ROOT / "docs/parity/source_control_installed_role_inventory_5_0_6.json.gz"
INVENTORY_SHA256 = "4fc6da2f72aa93046801231db226375093830f64fad3255d1c136d5194fcc350"
OBSERVER = ROOT / "tests/parity/python_execution_observer_5_0_6.py"
ROLE = "source_control_or_invariant_requires_disposition"
P0 = "parity_p0_inventory_5_0_6"


def require(condition, message):
    if not condition:
        raise RuntimeError("Source/control execution: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def properties(test):
    return {row["name"]: row["value"] for row in test["properties"]}


def validate_inventory(inventory, tests):
    require(inventory["registered_tests"] == len(tests) == 1101
            and len(inventory["rows"]) == 716, "fixed complete registry required")
    registered = {test["name"]: test for test in tests}
    require(len(registered) == len(tests), "unique CTest names required")
    for name, wanted in inventory.get("support_files", {}).items():
        require(digest(ROOT / name) == wanted, "qualified receipt helper changed: " + name)
    for row in inventory["rows"]:
        require(row["test"] in registered
                and row["command"] == registered[row["test"]]["command"]
                and row["properties"] == registered[row["test"]]["properties"]
                and digest(ROOT / row["script"]) == row["script_sha256"],
                "registered command, properties or source identity changed: " + row["test"])
    selected = [row for row in inventory["rows"] if row["role"] == ROLE]
    require(len(selected) == len({row["test"] for row in selected}) == 203
            and len({row["script"] for row in selected}) == 178
            and sum(row["source_fixture"] for row in selected) == 67,
            "complete source/control/invariant cohort required")
    return selected


def inventory_controls(inventory, tests):
    results = []
    for case in ("missing_entry", "wrong_command", "wrong_source_hash", "wrong_role"):
        changed = copy.deepcopy(inventory)
        row = changed["rows"][0]
        if case == "missing_entry":
            changed["rows"].pop()
        elif case == "wrong_command":
            row["command"].append("--controlled-wrong-selection")
        elif case == "wrong_source_hash":
            row["script_sha256"] = "0" * 64
        elif case == "wrong_role":
            row["role"] = "controlled-unknown-role"
        try:
            validate_inventory(changed, tests)
        except RuntimeError as error:
            results.append({"case": case, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Invalid inventory accepted: " + case)
    return results


def run(command, environment, timeout):
    started = time.monotonic()
    child = subprocess.Popen(command, cwd=ROOT, env=environment, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             start_new_session=True)
    timed_out = False
    try:
        stdout, stderr = child.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            stdout, stderr = child.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            stdout, stderr = child.communicate()
    except BaseException:
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            child.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.communicate()
        raise
    return {"command": command, "pid": child.pid, "exit_code": child.returncode,
            "stdout": stdout, "stderr": stderr,
            "duration_seconds": time.monotonic() - started, "timed_out": timed_out}


def environment(owned, logs, optimized, call_mode="calls"):
    value = dict(os.environ)
    value.update(CODEX_TEAQL_CHECK_OBSERVER_ROOT=str(ROOT),
                 CODEX_TEAQL_CHECK_OBSERVER_OWNED=str(owned),
                 CODEX_TEAQL_CHECK_OBSERVER_LOGS=str(logs),
                 CODEX_TEAQL_CHECK_OBSERVER_CALL_MODE=call_mode,
                 PYTHONDONTWRITEBYTECODE="1", TMPDIR=str(owned / "children"))
    value["PYTHONPATH"] = str(owned) + (os.pathsep + value["PYTHONPATH"]
                                        if value.get("PYTHONPATH") else "")
    if optimized:
        value["PYTHONOPTIMIZE"] = "1"
    else:
        value.pop("PYTHONOPTIMIZE", None)
    return value


def observer_controls(owned, results):
    function = 'def probe(value):\n    assert value, "OBSERVED_FIXTURE_ASSERTION"\n'
    for kind in ("file", "dynamic", "file_alias"):
        fixture = owned / ("observer_fixture_" + kind + ".py")
        name = "<teaql_observer_fixture_dynamic>" if kind == "dynamic" else str(fixture)
        body = function if kind == "file" else "exec(compile(" + repr(function) + ", " + repr(name) + ", 'exec'))\n"
        fixture.write_text('import sys\n' + body
                           + 'probe(sys.argv[1] == "valid")\nprint("OBSERVED_FIXTURE_RETURN")\n')
        for optimized in (False, True):
            for case in ("valid", "invalid"):
                logs = owned / ("preflight_" + kind + "_" + str(optimized) + "_" + case)
                logs.mkdir()
                command = [sys.executable, "-B", *(["-O"] if optimized else []), str(fixture), case]
                result = run(command, environment(owned, logs, optimized), 60)
                traces = list(logs.glob("*.json"))
                require(len(traces) == 1, "one actual observer preflight process required")
                trace = json.loads(traces[0].read_text())
                codes = [row for row in trace["called_assertion_file_code"]
                         if row["path"] == name and row["name"] == "probe"]
                wanted = 0 if optimized or case == "valid" else 1
                result.update(optimized=optimized, case=case, kind=kind, trace=trace)
                results.append(result)
                entry = {"script": str(fixture), "command": command, "properties": []}
                require(root_trace_acceptance([entry], [trace], trace["parent_pid"], optimized),
                        "actual interpreter-option/sys.argv observation")
                require(result["exit_code"] == wanted and trace["optimized"] == int(optimized)
                        and trace["observer_sha256"] == digest(OBSERVER)
                        and not trace["observer_errors"]
                        and len(codes) == 1 and codes[0]["calls"] == 1
                        and codes[0]["assertion_opcodes"] == (0 if optimized else 1),
                        "actual observer result or assertion opcode observation changed")
    fixture = owned / "observer_thread_fixture.py"
    fixture.write_text('import subprocess,sys\nfrom concurrent.futures import ThreadPoolExecutor\n'
                       'import sitecustomize as observer\nobserver.AUDIT_STATE.active=True\n'
                       'with ThreadPoolExecutor(max_workers=4) as pool:\n'
                       '    list(pool.map(lambda _: subprocess.run([sys.executable,"-I","-S","-c","pass"],check=True),range(8)))\n'
                       'observer.AUDIT_STATE.active=False\n')
    for optimized in (False, True):
        logs = owned / ("preflight_threads_" + str(optimized))
        logs.mkdir()
        command = [sys.executable, "-B", *(["-O"] if optimized else []), str(fixture)]
        result = run(command, environment(owned, logs, optimized), 60)
        traces = [json.loads(path.read_text()) for path in logs.glob("*.json")]
        result.update(optimized=optimized, kind="threaded_subprocesses", traces=traces)
        results.append(result)
        require(result["exit_code"] == 0 and len(traces) == 1
                and len(traces[0]["subprocesses"]) == 8
                and not traces[0]["observer_errors"],
                "thread-local observer recursion state must retain every worker spawn")
    return results


def observer_audit_controls(owned, results):
    for optimized in (False, True):
        for case in ("valid", "invalid"):
            fixture = owned / "observer_fixture_dynamic.py"
            logs = owned / ("audit_dynamic_" + str(optimized) + "_" + case)
            logs.mkdir()
            command = [sys.executable, "-B", *(["-O"] if optimized else []), str(fixture), case]
            result = run(command, environment(owned, logs, optimized, "audit_only"), 60)
            traces = [json.loads(path.read_text()) for path in logs.glob("*.json")]
            result.update(optimized=optimized, case=case, kind="audit_only_dynamic", traces=traces)
            results.append(result)
            wanted = 0 if optimized or case == "valid" else 1
            require(result["exit_code"] == wanted and len(traces) == 1,
                    "audit-only observation must preserve actual valid/invalid exits")
            trace = traces[0]
            entry = {"script": str(fixture), "command": command, "properties": []}
            require(root_trace_acceptance([entry], traces, trace["parent_pid"], optimized, "audit_only")
                    and trace["called_assertion_file_code"] == [],
                    "audit-only mode must preserve hooks and declare absent function observations")
            dynamic = "<teaql_observer_fixture_dynamic>"
            require(any(row["path"] == dynamic for row in trace["compiles"])
                    and any(row["path"] == dynamic for row in trace["execs"])
                    and any(row["path"] == str(fixture) for row in trace["execs"]),
                    "audit-only mode must retain explicit dynamic compilation and execution")
        fixture = owned / "observer_thread_fixture.py"
        logs = owned / ("audit_threads_" + str(optimized))
        logs.mkdir()
        command = [sys.executable, "-B", *(["-O"] if optimized else []), str(fixture)]
        result = run(command, environment(owned, logs, optimized, "audit_only"), 60)
        traces = [json.loads(path.read_text()) for path in logs.glob("*.json")]
        result.update(optimized=optimized, kind="audit_only_threads", traces=traces)
        results.append(result)
        entry = {"script": str(fixture), "command": command, "properties": []}
        require(result["exit_code"] == 0 and len(traces) == 1
                and root_trace_acceptance([entry], traces, traces[0]["parent_pid"], optimized, "audit_only")
                and len(traces[0]["subprocesses"]) == 8
                and traces[0]["called_assertion_file_code"] == [],
                "audit-only mode must retain every actual threaded child spawn")


def python_entry(row):
    command = row["command"]
    script = str(ROOT / row["script"])
    require(command.count(script) == 1, "one Python entry script argument required")
    index = command.index(script)
    require(index > 0 and set(command[1:index]) <= {"-B", "-O", "-OO", "-E", "-I"},
            "declared Python interpreter options required")
    return command[1:index], command[index:]


def expected_mode(row, optimized):
    options, _ = python_entry(row)
    if "-OO" in options:
        return 2
    if "-O" in options:
        return 1
    if "-E" in options or "-I" in options:
        return 0
    value = "1" if optimized else "0"
    for setting in properties(row).get("ENVIRONMENT", []):
        if setting.startswith("PYTHONOPTIMIZE="):
            value = setting.split("=", 1)[1]
    return int(value or "0")


def root_trace_acceptance(rows, traces, parent_pid, optimized, call_mode="calls"):
    roots = [trace for trace in traces if trace["parent_pid"] == parent_pid]
    if not all(type(trace["optimized"]) is int
               and trace.get("call_mode") == call_mode
               and trace["observer_sha256"] == digest(OBSERVER)
               and trace["observer_errors"] == []
               and trace["profile_retained"] is True
               and trace["trace_retained"] is True for trace in roots):
        return False
    expected = Counter((tuple(python_entry(row)[1]), expected_mode(row, optimized)) for row in rows)
    actual = Counter((tuple(trace["initial_argv"]), trace["optimized"]) for trace in roots)
    return actual == expected


def ctest_run(rows, owned, optimized, call_mode="calls"):
    logs = owned / ("optimized" if optimized else "ordinary")
    logs.mkdir()
    xml = logs / "ctest.xml"
    names = {row["test"] for row in rows}
    command = ["ctest", "--preset", "native", "--output-on-failure", "-j", "4", "-R",
               "^(" + "|".join(re.escape(name) for name in sorted(names)) + ")$",
               "--output-junit", str(xml)]
    result = run(command, environment(owned, logs, optimized, call_mode), 2400)
    result["optimized"] = optimized
    result["call_mode"] = call_mode
    result["junit"] = xml.read_text() if xml.exists() else None
    result["traces"] = [json.loads(path.read_text()) for path in sorted(logs.glob("*.json"))]
    result["junit_accepted"] = False
    if result["junit"] is not None:
        try:
            document = ET.fromstring(result["junit"])
            cases = list(document.iter("testcase"))
            result["junit_accepted"] = (result["exit_code"] == 0 and len(cases) == len(names)
                and {case.get("name") for case in cases} == names
                and all(case.get("status") == "run" for case in cases)
                and not any(list(document.iter(tag)) for tag in ("failure", "error", "skipped")))
        except ET.ParseError as error:
            result["junit_error"] = str(error)
    roots = [trace for trace in result["traces"] if trace["parent_pid"] == result["pid"]]
    result["root_trace_count"] = len(roots)
    result["root_trace_accepted"] = root_trace_acceptance(
        rows, result["traces"], result["pid"], optimized, call_mode)
    result["selected"] = rows
    return result


def native_hashes():
    result = {}
    for path in (ROOT / "build_native_current").iterdir():
        if path.is_file():
            with path.open("rb") as handle:
                is_elf = handle.read(4) == b"\x7fELF"
            if is_elf:
                result[path.name] = digest(path)
    return result


def evidence_hashes(excluded):
    return {path.relative_to(ROOT).as_posix(): digest(path)
            for directory in (ROOT / "docs/parity", ROOT / "tests/parity")
            for path in sorted(directory.rglob("*.json.gz")) if path != excluded}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--preflight-only", action="store_true")
    parser.add_argument("--mode", choices=("both", "ordinary", "optimized"), default="both")
    parser.add_argument("--observation", choices=("calls", "audit_only"), default="calls",
                        help="Use audit-only events for large controls; qualify called helpers separately")
    parser.add_argument("--tests", nargs="+", help="Explicit registered source/control subset; fixtures remain required")
    args = parser.parse_args()
    record = args.record.resolve()
    require(not record.exists(), "preserve earlier observation receipts")
    require(digest(INVENTORY) == INVENTORY_SHA256, "fixed role inventory changed")
    inventory = read(INVENTORY)
    tests = json.loads(subprocess.check_output(
        ["ctest", "--preset", "native", "--show-only=json-v1"], cwd=ROOT, text=True))["tests"]
    rows = validate_inventory(inventory, tests)
    requested = {row["test"] for row in rows} if args.tests is None else set(args.tests)
    require(bool(requested) and requested <= {row["test"] for row in rows},
            "nonempty known requested registrations required")
    report = {"complete": False, "whole_goal_complete": False, "complete_sq02": False,
              "scope": "Observer preflight only" if args.preflight_only else
                  "Actual registered source/control/invariant execution and observed Python boundaries; per-boundary semantic dispositions remain separate",
              "inventory_sha256": INVENTORY_SHA256, "selected": rows,
              "requested_tests": sorted(requested), "requested_mode": args.mode,
              "requested_observation": args.observation,
              "runtime_commit": inventory["runtime_commit"], "generator_commit": inventory["generator_commit"],
              "collector_source": Path(__file__).read_text(), "collector_sha256": digest(Path(__file__)),
              "observer_source": OBSERVER.read_text(), "observer_sha256": digest(OBSERVER),
              "inventory_controls": inventory_controls(inventory, tests), "runs": []}
    inputs = capture_inputs()
    report["input_hashes"] = inputs
    report["references_before"] = references()
    if not args.preflight_only:
        report["native_elf_files"] = native_hashes()
        report["evidence_inputs"] = evidence_hashes(record)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    error = None
    with tempfile.TemporaryDirectory(prefix="teaql_source_control_execution_506_") as directory:
        owned = Path(directory)
        (owned / "sitecustomize.py").write_bytes(OBSERVER.read_bytes())
        (owned / "children").mkdir()
        try:
            report["observer_controls"] = []
            observer_controls(owned, report["observer_controls"])
            observer_audit_controls(owned, report["observer_controls"])
            if not args.preflight_only:
                modes = (False, True) if args.mode == "both" else (args.mode == "optimized",)
                for optimized in modes:
                    wanted = requested - ({P0} if optimized else set())
                    require(bool(wanted), "requested mode must contain executable registrations")
                    expression = "^(" + "|".join(re.escape(name) for name in sorted(wanted)) + ")$"
                    expanded = json.loads(subprocess.check_output(
                        ["ctest", "--preset", "native", "--show-only=json-v1", "-R", expression],
                        cwd=ROOT, text=True))["tests"]
                    expanded_names = {test["name"] for test in expanded}
                    require(expanded_names <= {row["test"] for row in rows}
                            and (not optimized or P0 not in expanded_names),
                            "fixture closure must stay in the declared cohort and mode")
                    selected = [row for row in rows if row["test"] in expanded_names]
                    print("Running " + str(len(selected)) + " source/control CTests; optimized="
                          + str(optimized) + "; observation=" + args.observation, flush=True)
                    result = ctest_run(selected, owned, optimized, args.observation)
                    report["runs"].append(result)
                    record.write_bytes(gzip.compress(json.dumps(report, sort_keys=True).encode(), mtime=0))
                    require(result["junit_accepted"], "actual CTest selection failed")
                    require(result["root_trace_accepted"], "complete unaltered entry observations required")
                    print(str(len(selected)) + " source/control CTests and root traces pass; optimized="
                          + str(optimized), flush=True)
                logs = owned / "p0_optimized_refusal"
                logs.mkdir()
                p0 = next(row for row in rows if row["test"] == P0)
                refusal = run([sys.executable, "-B", "-O", *p0["command"][1:]],
                              environment(owned, logs, True), 60)
                refusal["traces"] = [json.loads(path.read_text()) for path in logs.glob("*.json")]
                report["p0_optimized_refusal"] = refusal
                require(refusal["exit_code"] != 0
                        and "run the inventory gate with active checks" in refusal["stderr"],
                        "P0 ordinary-only execution boundary must reject optimization")
        except BaseException:
            error = traceback.format_exc()
        finally:
            report["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*")
                                                if path.is_file())
    report["owned_directory_removed"] = not owned.exists()
    report["inputs_unchanged"] = capture_inputs() == inputs
    report["references_after"] = references()
    if not args.preflight_only:
        report["native_elf_unchanged"] = native_hashes() == report["native_elf_files"]
        report["evidence_unchanged"] = evidence_hashes(record) == report["evidence_inputs"]
    report["error"] = error
    report["complete"] = (error is None and report["owned_directory_removed"]
        and report["inputs_unchanged"] and report["references_after"] == report["references_before"]
        and (args.preflight_only or report["native_elf_unchanged"] and report["evidence_unchanged"]))
    record.write_bytes(gzip.compress(json.dumps(report, sort_keys=True).encode(), mtime=0))
    if error:
        print(error, file=sys.stderr)
    require(report["complete"], "observation did not qualify; inspect retained receipt")
    print("Source/control observation complete in its declared scope; full SQ02 remains separate")


if __name__ == "__main__":
    main()
