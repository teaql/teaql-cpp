#!/usr/bin/env python3
"""Qualify the new narrow-codec source fixture and its current native consumers.

This extends registration coverage. It does not replay or promote the frozen
716-command SQ02 execution, or accept the complete generator/runtime goal.
"""

import argparse
from collections import Counter
import copy
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from collect_source_control_execution_5_0_6 import native_hashes
from narrow_integer_registration_source_admission_5_0_6 import initial, sources
from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, read, references, require, sha, write

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_narrow_integer_codec_5_0_6 import corpus
from compare_values import same
from frontend_bindings_5_0_6 import binding_properties

EVIDENCE = ROOT / "docs/parity"
INVENTORY = EVIDENCE / "source_control_installed_role_inventory_5_0_6.json.gz"
INVENTORY_SHA = "4fc6da2f72aa93046801231db226375093830f64fad3255d1c136d5194fcc350"
COMPARISON = "narrow_integer_codec_rust_parity"
FIXTURE = "narrow_integer_codec_source_contracts"
PROGRAM = "teaql_narrow_integer_codec_probe"
IMPORTED = "teaql_module_narrow_integer_codec_probe"


def properties(test):
    return {row["name"]: row["value"] for row in test["properties"]}


def registry(preset, selection=None):
    command = ["ctest", "--preset", preset, "--show-only=json-v1"]
    if selection is not None:
        command.extend(["-R", selection])
    return json.loads(subprocess.check_output(command, cwd=ROOT, text=True))["tests"]


def signature(test):
    return copy.deepcopy(dict(command=test["command"], properties=properties(test)))


def validate_registry(native, module):
    for preset, tests, expected_count, prefix in (("native", native, 1103, ""),
                                                   ("modules_native", module, 787, "module_")):
        actual = {row["name"]: row for row in tests}
        prior = {row["name"]: row for row in initial()["native_registry" if not prefix else "module_registry"]}
        require(len(actual) == len(tests) == expected_count
                and actual.keys() == prior.keys() | {prefix + FIXTURE}, "one new unique source fixture: " + preset)
        for name, row in prior.items():
            expected = signature(row)
            if name == prefix + COMPARISON:
                expected["properties"].setdefault("FIXTURES_REQUIRED", []).insert(0, prefix + "narrow_integer_codec")
                expected["properties"].setdefault("DEPENDS", []).insert(0, prefix + FIXTURE)
            require(signature(actual[name]) == expected, "preserved command and exact prerequisite delta: " + name)
    native_fixture = next(row for row in native if row["name"] == FIXTURE)
    python = native_fixture["command"][0]
    require(native_fixture["command"] == [python, "-B", str(ROOT / "scripts/narrow_integer_source_admission_5_0_6.py"), "--check"]
            and properties(native_fixture) == dict(FIXTURES_SETUP=["narrow_integer_codec"], RUN_SERIAL=True,
                                                  TIMEOUT=180.0, WORKING_DIRECTORY=str(ROOT / "build_native_current")),
            "actual complete-source admission fixture command and policy")
    imported_fixture = next(row for row in module if row["name"] == "module_" + FIXTURE)
    require(imported_fixture["command"] == native_fixture["command"]
            and imported_fixture["properties"] == binding_properties(native_fixture["properties"], ROOT / "build_modules_native", "module"),
            "canonical imported fixture delegates to the same unchanged source gate")
    require(sha(INVENTORY.read_bytes()) == INVENTORY_SHA, "immutable historical role inventory")
    inventory = read(INVENTORY)
    old = {row["test"]: row for row in inventory["rows"]}
    python_rows = {row["name"]: row for row in native if any(part.endswith(".py") for part in row["command"])}
    require(len(old) == 716 and len(python_rows) == 718
            and python_rows.keys() == old.keys() | {COMPARISON, FIXTURE}, "complete disjoint current Python registration partition")
    roles = Counter(row["role"] for row in old.values())
    roles["named_custom_comparison"] += 1
    roles["source_control_or_invariant_requires_disposition"] += 1
    setup = {}
    for row in native:
        for group in properties(row).get("FIXTURES_SETUP", []):
            require(group not in setup, "unique registered source-fixture owner")
            setup[group] = row["name"]
    owners = set()
    for name, role in [(name, row["role"]) for name, row in old.items()] + [(COMPARISON, "named_custom_comparison")]:
        if role == "source_control_or_invariant_requires_disposition":
            continue
        required = properties(python_rows[name]).get("FIXTURES_REQUIRED", [])
        require(required and all(group in setup for group in required), "every comparison has registered source prerequisites: " + name)
        owners.update(setup[group] for group in required)
    return dict(registered_tests=len(native), python_commands=len(python_rows),
                python_entry_files=len({part for row in python_rows.values() for part in row["command"] if part.endswith(".py")}),
                roles=dict(roles), comparison_commands=514, source_control_commands=204,
                source_fixture_owners=len(owners), historical_python_commands=716,
                fresh_full_sq02_execution=False, complete_current_sq02=False)


def selection():
    previous = initial()
    targets = sorted(previous["old_native_changed"] + previous["old_native_added"])
    require(len(targets) == 10 and previous["old_native_removed"] == [] and PROGRAM in targets,
            "nine changed historical programs and the new narrow observer")
    tests = previous["native_registry"]
    selected = set()
    for target in targets:
        consumers = {row["name"] for row in tests if any(Path(part).name == target for part in row["command"])}
        require(consumers, "named registered caller for changed program: " + target)
        selected.update(consumers)
    entries = read(INVENTORY)["rows"]
    changed_guards = {row["test"] for row in entries
                      if previous["inputs_after"]["runtime:" + row["script"]] != row["script_sha256"]}
    require(len(changed_guards) == 8, "exact eight source/control entry changes since frozen SQ02")
    selected.update(changed_guards)
    selected.update((COMPARISON, FIXTURE))
    return targets, "^(" + "|".join(re.escape(name) for name in sorted(selected)) + ")$"


def run(steps, label, command, environment, input_text=None):
    command = list(map(str, command))
    print(label, flush=True)
    with subprocess.Popen(command, cwd=ROOT, env=environment, text=True, stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True) as child:
        try:
            stdout, stderr = child.communicate(input=input_text, timeout=1800)
        except BaseException:
            if child.poll() is None:
                os.killpg(child.pid, signal.SIGTERM)
                try:
                    child.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(child.pid, signal.SIGKILL)
                    child.communicate()
            raise
    step = dict(label=label, command=command, exit_code=child.returncode,
                stdout=stdout, stderr=stderr, optimize=environment.get("PYTHONOPTIMIZE", "0"))
    if input_text is not None:
        step["input"] = input_text
    steps.append(step)
    require(type(step["exit_code"]) is int and step["exit_code"] == 0, "successful actual command: " + label)
    print(label, "passed", flush=True)
    return stdout


def execute(path):
    require(not path.exists(), "preserve earlier registration execution")
    targets, pattern = selection()
    source_frame = execution_inputs()
    record = dict(complete=False, whole_goal_complete=False, complete_current_sq02=False, steps=[],
                  inputs_before=source_frame, references_before=references(), native_inputs_before=native_inputs(),
                  native_registry=registry("native"), module_registry=registry("modules_native"),
                  native_elf_before=native_hashes(), imported_before=sha((ROOT / "build_modules_native/modules" / IMPORTED).read_bytes()),
                  targets=targets, pattern=pattern, junit={}, selected={})
    environment = dict(os.environ, PYTHONOPTIMIZE="0", PYTHONDONTWRITEBYTECODE="1")
    owned = None
    try:
        validate_registry(record["native_registry"], record["module_registry"])
        with tempfile.TemporaryDirectory(prefix="teaql_narrow_source_registration_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                native_build = ["cmake", "--build", "--preset", "native", "--target", *targets, "--parallel", "2"]
                module_build = ["cmake", "--build", "--preset", "modules_native", "--target", IMPORTED, "--parallel", "2"]
                run(record["steps"], "native_build", native_build, environment)
                run(record["steps"], "module_build", module_build, environment)
                for mode in (0, 1):
                    for preset, selected in (("native", pattern), ("modules_native", "^module_" + COMPARISON + "$")):
                        key = preset + "_" + str(mode)
                        record["selected"][key] = sorted(row["name"] for row in registry(preset, selected))
                        command = ["ctest", "--preset", preset, "-R", selected, "--output-on-failure",
                                   "--output-junit", str(owned / (key + ".xml"))]
                        run(record["steps"], key, command, dict(environment, PYTHONOPTIMIZE=str(mode)))
                        record["junit"][key] = (owned / (key + ".xml")).read_text()
                data = "".join(json.dumps(row["input"], sort_keys=True) + "\n" for row in corpus()["cases"])
                for label, program in (("native_raw", ROOT / "build_native_current" / PROGRAM),
                                       ("module_raw", ROOT / "build_modules_native/modules" / IMPORTED)):
                    output = run(record["steps"], label, [program], environment, data)
                    record[label] = [json.loads(line) for line in output.splitlines() if line.strip()]
                run(record["steps"], "native_no_work", native_build, environment)
                run(record["steps"], "module_no_work", module_build, environment)
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(item.stat().st_size for item in owned.rglob("*") if item.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(inputs_after=execution_inputs(), references_after=references(),
                      native_inputs_after=native_inputs(), native_elf_after=native_hashes(),
                      imported_after=sha((ROOT / "build_modules_native/modules" / IMPORTED).read_bytes()),
                      owned_directory_removed=owned is not None and not owned.exists())
        write(path, record)


def validate(record, current):
    from compiler_offline_source_admission_5_0_6 import frame_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["complete_current_sq02"] is False, "bounded successful fixture/current-consumer execution")
    summary = validate_registry(record["native_registry"], record["module_registry"])
    require(record["native_registry"] == current["native_registry"] and record["module_registry"] == current["module_registry"],
            "actual current complete registration snapshots")
    require(record["inputs_before"] == record["inputs_after"]
            and frame_matches(current["inputs"], record["inputs_after"])
            and frame_matches(current["inputs"], initial()["inputs_after"]), "exact stable qualified registration input epoch")
    require(record["references_before"] == record["references_after"] == current["references"] == initial()["references"], "exact locked references")
    require(record["native_inputs_before"] == record["native_inputs_after"] == current["native_inputs"] == initial()["native_inputs"],
            "unchanged shared runtime/compiler/SQLite artifacts")
    require(record["native_elf_before"] == initial()["native_elf_files"]
            and record["native_elf_after"] == current["native_elf_files"]
            and record["native_elf_before"].keys() == record["native_elf_after"].keys(), "complete before/current native program identities")
    targets, pattern = selection()
    require(record["targets"] == targets and record["pattern"] == pattern, "all changed historical consumers and eight entry guards selected")
    require(all(record["native_elf_before"][name] == record["native_elf_after"][name]
                for name in record["native_elf_before"] if name not in targets), "unselected native artifacts remain exact")
    require(record["imported_before"] == record["imported_after"] == current["imported"], "unchanged actual canonical imported observer")
    owned = Path(record["owned_directory"])
    require(record["owned_directory_removed"] is True and not owned.exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0, "reaped task-owned JUnit directory is removed")
    native_build = ["cmake", "--build", "--preset", "native", "--target", *targets, "--parallel", "2"]
    module_build = ["cmake", "--build", "--preset", "modules_native", "--target", IMPORTED, "--parallel", "2"]
    expected = [("native_build", native_build, "0"), ("module_build", module_build, "0")]
    for mode in (0, 1):
        for preset, selected in (("native", pattern), ("modules_native", "^module_" + COMPARISON + "$")):
            key = preset + "_" + str(mode)
            names = current["selected"][key]
            require(record["selected"][key] == names and len(names) == len(set(names)), "exact fixture-expanded selected registry: " + key)
            cases = list(ET.fromstring(record["junit"][key]).iter("testcase"))
            require(sorted(row.attrib["name"] for row in cases) == names
                    and not any(row.find(tag) is not None for row in cases for tag in ("failure", "error", "skipped")),
                    "exact successful JUnit membership: " + key)
            source_name, comparison_name = ("" if preset == "native" else "module_") + FIXTURE, ("" if preset == "native" else "module_") + COMPARISON
            output = next(row["stdout"] for row in record["steps"] if row["label"] == key)
            completed = list(re.finditer(r"^\s*\d+/\d+\s+Test\s+#\d+:\s+" + re.escape(source_name)
                                         + r"\s+\.{2,}\s+Passed\b", output, flags=re.MULTILINE))
            started = list(re.finditer(r"^\s*Start\s+\d+:\s+" + re.escape(comparison_name)
                                       + r"\s*$", output, flags=re.MULTILINE))
            require(len(completed) == len(started) == 1 and completed[0].start() < started[0].start(),
                    "actual source completion precedes the comparison start; JUnit order is not execution order")
            expected.append((key, ["ctest", "--preset", preset, "-R", selected, "--output-on-failure",
                                   "--output-junit", str(owned / (key + ".xml"))], str(mode)))
    for label, program in (("native_raw", ROOT / "build_native_current" / PROGRAM),
                           ("module_raw", ROOT / "build_modules_native/modules" / IMPORTED)):
        expected.append((label, [str(program)], "0"))
    expected.extend((("native_no_work", native_build, "0"), ("module_no_work", module_build, "0")))
    require([row["label"] for row in record["steps"]] == [row[0] for row in expected], "all actual selected build, test, observer and no-work steps")
    for step, (label, command, mode) in zip(record["steps"], expected, strict=True):
        require(step["command"] == command and step["optimize"] == mode
                and type(step["exit_code"]) is int and step["exit_code"] == 0, "exact terminal command and Python mode: " + label)
    require(all("ninja: no work to do." in row["stdout"] for row in record["steps"][-2:]), "both final selected builds have no work")
    rows = corpus()["cases"]
    data = "".join(json.dumps(row["input"], sort_keys=True) + "\n" for row in rows)
    expected_values = [row["expected"] for row in rows]
    for key in ("native_raw", "module_raw"):
        step = next(row for row in record["steps"] if row["label"] == key)
        actual = [json.loads(line) for line in step["stdout"].splitlines() if line.strip()]
        require(step["input"] == data and len(actual) == 376 and same(actual, record[key]) and same(actual, expected_values),
                "complete authentic narrow integer values and errors: " + key)
    return summary


def controls(record, current):
    mutations = (
        ("incomplete", lambda row: row.update(complete=False)),
        ("promoted_sq02", lambda row: row.update(complete_current_sq02=True)),
        ("missing_fixture", lambda row: row.update(native_registry=[item for item in row["native_registry"] if item["name"] != FIXTURE])),
        ("changed_native_hash", lambda row: row["native_elf_after"].update({PROGRAM: "0" * 64})),
        ("changed_imported_hash", lambda row: row.update(imported_after="0" * 64)),
        ("forged_command", lambda row: row["steps"][0].update(command=["true"])),
        ("boolean_exit", lambda row: row["steps"][0].update(exit_code=False)),
        ("wrong_python_mode", lambda row: row["steps"][4].update(optimize="0")),
        ("missing_junit_member", lambda row: row["junit"].update(native_0="<testsuite/>")),
        ("lost_original_output", lambda row: row["native_raw"].pop()),
        ("wrong_imported_value", lambda row: row["module_raw"].__setitem__(0, {"ok": True, "value": 999})),
        ("source_changed_during_execution", lambda row: row["inputs_after"].update({"runtime:tests/CMakeLists.txt": "0" * 64})),
        ("unsettled_final_build", lambda row: row["steps"][-1].update(stdout="linking")),
        ("cleanup_missing", lambda row: row.update(owned_directory_removed=False)),
        ("reversed_fixture_execution_order", lambda row: row["steps"][2].update(
            stdout="Start 594: narrow_integer_codec_rust_parity\n"
                   "1/38 Test #595: narrow_integer_codec_source_contracts .... Passed 1 sec\n")),
    )
    result = []
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed, current)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted registered narrow-integer corruption: " + name)
    return result


def current_state():
    _, pattern = selection()
    return dict(inputs=execution_inputs(), references=references(), native_inputs=native_inputs(),
                native_elf_files=native_hashes(), native_registry=registry("native"), module_registry=registry("modules_native"),
                imported=sha((ROOT / "build_modules_native/modules" / IMPORTED).read_bytes()),
                selected={preset + "_" + str(mode): sorted(row["name"] for row in registry(preset, selected))
                          for mode in (0, 1) for preset, selected in (("native", pattern), ("modules_native", "^module_" + COMPARISON + "$"))})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    sources()
    if not args.check:
        execute(args.record)
    record = read(args.record)
    current = current_state()
    summary = validate(record, current)
    if args.controls:
        require(not args.controls.exists(), "preserve current registration controls")
        write(args.controls, dict(complete=True, whole_goal_complete=False, complete_current_sq02=False,
                                 optimized=sys.flags.optimize, execution_sha256=sha(args.record.read_bytes()),
                                 summary=summary, controls=controls(record, current)))
    require(current["inputs"] == execution_inputs() and current["references"] == references(), "stable admission inputs")
    print(json.dumps(dict(summary=summary, native_selected=len(record["selected"]["native_0"]),
                          module_selected=len(record["selected"]["modules_native_0"]), raw_observations=752,
                          optimized=sys.flags.optimize), sort_keys=True), flush=True)


if __name__ == "__main__":
    main()
