#!/usr/bin/env python3
"""Admit the exact native/imported narrow-integer source-fixture registration."""

import argparse
import ast
import copy
from functools import cache
import json
from pathlib import Path
import sys

from verify_compiler_generated_business_5_0_6 import read, require, sha, write

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "docs/parity"
INITIAL = EVIDENCE / "current_narrow_source_registration_initial_5_0_6.json.gz"
INITIAL_SHA = "e48b0db9d4d7ebac6fdc83b4d6ff0e18375c75ad2eaad0ab2c5cfb5f9aaa782c"
SOURCE = EVIDENCE / "current_narrow_source_registration_scoped_sources_5_0_6.json.gz"
SOURCE_SHA = "a977f5201b63daccfec57ae85a81e1a6ac830009384f82dd99f06d37ee7abf51"
CONTINUATION = EVIDENCE / "current_narrow_source_registration_reader_sources_5_0_6.json.gz"
CONTINUATION_SHA = "dbc5f984c8f936e5a07e02358158ab94e779295fa418833f86c6ca1815de99a9"
CUSTOM = "scripts/verify_custom_comparison_bindings_5_0_6.py"
DATABASE = "build_native_current/compile_commands.json"
SQL = EVIDENCE / "ap02_const_function_sql_5_0_6.json.gz"
SQL_SHA = "5202a86d855aa21668053086f31ab39294fc97e12b038bf50d50cb3560c253dc"
NOMINAL = EVIDENCE / "current_narrow_source_registration_nominal_forwarding_5_0_6.json.gz"
NOMINAL_SHA = "af94b50b7e956ebc8827819a4da8e6c59a52414bd0febfb79aa38f3ffed20b76"
NOMINAL_HELPER = "scripts/nominal_type_source_admission_5_0_6.py"
READER_PREIMAGE = EVIDENCE / "current_narrow_source_registration_reader_preimage_5_0_6.json.gz"
READER_PREIMAGE_SHA = "98610adbfe2eff2c0aa660caa4b6ed735a522268fa3c0174939d82e4f5aad163"
READER = EVIDENCE / "current_narrow_source_registration_reader_correction_5_0_6.json.gz"
READER_SHA = "8d84cb6f43103b03190bb1515c6120bdde52a287401c7e7233153cb9ce883e97"
SELF = "scripts/narrow_integer_registration_source_admission_5_0_6.py"
DRIVER = "scripts/verify_narrow_integer_registered_join_5_0_6.py"
SOURCE_FIXTURE = "scripts/narrow_integer_source_admission_5_0_6.py"
GUARDS = {
    "scripts/compiler_codec_module_source_admission_5_0_6.py": ["validate_sources"],
    "scripts/compiler_offline_input_source_admission_5_0_6.py": ["validate_sources"],
    "scripts/compiler_offline_source_admission_5_0_6.py": ["source_epochs", "added_input_hashes"],
}
REGISTRATION = '''    if(NOT CMAKE_CROSSCOMPILING)
        add_test(NAME narrow_integer_codec_source_contracts COMMAND ${Python3_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/narrow_integer_source_admission_5_0_6.py --check)
        set_tests_properties(narrow_integer_codec_source_contracts PROPERTIES
            TIMEOUT 180 RUN_SERIAL TRUE FIXTURES_SETUP narrow_integer_codec)
        set_tests_properties(narrow_integer_codec_rust_parity PROPERTIES FIXTURES_REQUIRED narrow_integer_codec)
    endif()
'''


@cache
def initial():
    require(sha(INITIAL.read_bytes()) == INITIAL_SHA, "immutable missing-fixture diagnosis")
    return read(INITIAL)


def validate_sources(record):
    from generation_decision_source_admission_5_0_6 import current_input_matches as decision_matches
    expected = {"tests/CMakeLists.txt", "modules/source_contracts.json", DRIVER, *GUARDS}
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == expected, "exact registration, manifest and identity-guard inputs")
    for name, row in record["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha((ROOT / name).read_bytes())
                     or corrected_source_matches(name, row["after_sha256"])
                     or decision_matches(ROOT / name, row["after_sha256"])), "exact qualified registration source: " + name)
        if name == DRIVER:
            require(row["before_source"] is None and row["before_sha256"] is None
                    and "runtime:" + name not in initial()["inputs_after"], "one new bounded registration driver")
        else:
            require(row["before_sha256"] == sha(row["before_source"].encode())
                    == initial()["inputs_after"]["runtime:" + name], "exact existing source preimage: " + name)
    row = record["paths"]["tests/CMakeLists.txt"]
    require(row["after_source"].count(REGISTRATION) == 1
            and row["after_source"].replace(REGISTRATION, "", 1) == row["before_source"],
            "only one source fixture and its comparator prerequisite are added")
    for name, changed in GUARDS.items():
        row = record["paths"][name]
        trees = [ast.parse(row[key]) for key in ("before_source", "after_source")]
        functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)}
                     for tree in trees]
        require(functions[0].keys() == functions[1].keys()
                and [key for key in functions[0] if functions[0][key] != functions[1][key]] == changed,
                "only reviewed identity functions change: " + name)
        for tree in trees:
            for node in tree.body:
                if isinstance(node, ast.FunctionDef) and node.name in changed:
                    node.body = [ast.Pass()]
        require(ast.dump(trees[0]) == ast.dump(trees[1]), "all execution/comparison/control bodies remain exact")
    row = record["paths"]["modules/source_contracts.json"]
    before, after = [json.loads(row[key]) for key in ("before_source", "after_source")]
    require(len(before["bindings"]) == 404 and len(before["fixtures"]) == 62,
            "complete preceding imported bindings and fixtures")
    before["native_registration_sha256"] = record["paths"]["tests/CMakeLists.txt"]["after_sha256"]
    binding = next(row for row in before["bindings"] if row["native"] == "narrow_integer_codec_rust_parity")
    require(binding["properties"]["FIXTURES_REQUIRED"] == ["module_source_contract_bindings"],
            "actual missing imported source prerequisite")
    binding["properties"]["FIXTURES_REQUIRED"].insert(0, "module_narrow_integer_codec")
    binding["properties"]["DEPENDS"].insert(0, "module_narrow_integer_codec_source_contracts")
    fixture = dict(test="module_narrow_integer_codec_source_contracts", native="narrow_integer_codec_source_contracts",
                   command=["@PYTHON@", "-B", "@SOURCE@/" + SOURCE_FIXTURE, "--check"],
                   properties=dict(DEPENDS=["module_source_contract_bindings"],
                                   FIXTURES_REQUIRED=["module_source_contract_bindings"],
                                   FIXTURES_SETUP=["module_narrow_integer_codec"], RUN_SERIAL=True,
                                   TIMEOUT=180.0, WORKING_DIRECTORY="@BUILD@/modules"))
    before["fixtures"].append(fixture)
    before["fixtures"].sort(key=lambda row: row["native"])
    before["source_groups"].append("narrow_integer_codec")
    before["source_groups"].sort()
    require(SOURCE_FIXTURE not in before["source_programs"]
            and sha((ROOT / SOURCE_FIXTURE).read_bytes()) == initial()["inputs_after"]["runtime:" + SOURCE_FIXTURE],
            "existing unchanged original-source admission becomes a prerequisite")
    before["source_programs"][SOURCE_FIXTURE] = sha((ROOT / SOURCE_FIXTURE).read_bytes())
    require(before == after, "only the exact fixture, comparator prerequisite and declaration identity change")


@cache
def sources():
    require(sha(SOURCE.read_bytes()) == SOURCE_SHA, "immutable exact registration source transition")
    record = read(SOURCE)
    validate_sources(record)
    return record["paths"]


def current_source_matches(path, expected):
    from generation_decision_source_admission_5_0_6 import current_input_matches as decision_matches
    if decision_matches(path, expected):
        return True
    path = Path(path).resolve()
    if not path.is_file() or type(expected) is not str:
        return False
    actual = sha(path.read_bytes())
    if expected == actual:
        return True
    if not path.is_relative_to(ROOT):
        return False
    if corrected_source_matches(path.relative_to(ROOT).as_posix(), expected):
        return True
    row = sources().get(path.relative_to(ROOT).as_posix())
    return row is not None and row["before_sha256"] == expected and (
        row["after_sha256"] == actual or decision_matches(path, row["after_sha256"]))


def source_rows():
    return {"runtime:" + name: row for name, row in {
        **sources(), **continuation_sources(), **nominal_sources(), **reader_sources()}.items()}


def validate_reader_correction(record):
    require(sha(READER_PREIMAGE.read_bytes()) == READER_PREIMAGE_SHA, "immutable completed execution and reader preimage")
    preimage = read(READER_PREIMAGE)
    execution_path = ROOT / preimage["execution_receipt"]
    require(sha(execution_path.read_bytes()) == preimage["execution_sha256"], "actual successful build/test/observer receipt")
    execution = read(execution_path)
    require(execution["complete"] is True and execution["inputs_before"] == execution["inputs_after"]
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0 for row in execution["steps"]),
            "completed business commands are reused without rerunning them for a reader correction")
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {DRIVER}, "one result reader correction")
    row = record["paths"][DRIVER]
    require(row["before_source"] == preimage["source_images"][DRIVER]["source"]
            and row["before_sha256"] == sha(row["before_source"].encode())
            == execution["inputs_after"]["runtime:" + DRIVER]
            and row["after_sha256"] == sha(row["after_source"].encode()) == sha((ROOT / DRIVER).read_bytes()),
            "exact executed/current reader source")
    trees = [ast.parse(row[key]) for key in ("before_source", "after_source")]
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)} for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and [name for name in functions[0] if functions[0][name] != functions[1][name]] == ["validate", "controls"],
            "only result admission and one new execution-order corruption change")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in ("validate", "controls"):
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "all actual execution, build, command and cleanup bodies remain exact")
    before = record["helper_before_source"]
    require(before == preimage["source_images"][SELF]["source"]
            and sha(before.encode()) == execution["inputs_after"]["runtime:" + SELF], "exact executed source-admission helper")
    from generation_decision_source_admission_5_0_6 import previous_source
    current = previous_source("runtime:" + SELF, Path(__file__).read_text())
    binding = 'READER_SHA = "' + READER_SHA + '"\n'
    require(current.count(binding) == 1
            and current.replace(binding, 'READER_SHA = "__READER_RECORD_SHA256__"\n', 1)
                == record["helper_after_source_normalized"],
            "exact current helper image; only its own immutable record hash is normalized")


@cache
def reader_sources():
    require(sha(READER.read_bytes()) == READER_SHA, "immutable exact result-reader correction")
    record = read(READER)
    validate_reader_correction(record)
    before = record["helper_before_source"]
    from generation_decision_source_admission_5_0_6 import previous_source
    after = previous_source("runtime:" + SELF, Path(__file__).read_text())
    return {**record["paths"], SELF: dict(before_source=before, before_sha256=sha(before.encode()),
                                         after_source=after, after_sha256=sha(after.encode()))}


def corrected_source_matches(name, expected):
    from generation_decision_source_admission_5_0_6 import current_input_matches as decision_matches
    row = reader_sources().get(name)
    return row is not None and row["before_sha256"] == expected and (
        row["after_sha256"] == sha((ROOT / name).read_bytes())
        or decision_matches(ROOT / name, row["after_sha256"]))


def validate_nominal_source(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {NOMINAL_HELPER}, "exact historical runtime/caller source forwarding")
    row = record["paths"][NOMINAL_HELPER]
    require(row["before_sha256"] == sha(row["before_source"].encode())
            == initial()["inputs_after"]["runtime:" + NOMINAL_HELPER]
            and row["after_sha256"] == sha(row["after_source"].encode())
            == sha((ROOT / NOMINAL_HELPER).read_bytes()), "exact preceding/current nominal admission source")
    edits = (
        ('        require(sha((ROOT / name).read_bytes()) == wanted, "current runtime source for SQL reuse")\n',
         '        require(latest_input_matches(ROOT / name, wanted), "qualified runtime source for SQL reuse")\n'),
        ('    from compiler_source_service_source_admission_5_0_6 import current_input_matches as service_input_matches\n'
         '    return service_input_matches(path, expected)\n',
         '    from compiler_projection_source_admission_5_0_6 import current_input_matches as projection_input_matches\n'
         '    return projection_input_matches(path, expected)\n'),
        ('    from compiler_scalar_source_admission_5_0_6 import source_epochs\n',
         '    from compiler_offline_source_admission_5_0_6 import source_epochs\n'),
        ('def cross_context_input_matches(path, expected):\n',
         'def cross_context_input_matches(path, expected):\n'
         '    if path == ROOT / "scripts/verify_audit_metadata_source_5_0_6.py":\n'
         '        return corpus_input_matches(path, expected)\n'),
    )
    restored = row["after_source"]
    for before, after in edits:
        require(restored.count(after) == 1, "one exact runtime/caller/source-graph forwarding edit")
        restored = restored.replace(after, before, 1)
    require(restored == row["before_source"], "all original validation and corruption-control bodies remain exact")


@cache
def nominal_sources():
    require(sha(NOMINAL.read_bytes()) == NOMINAL_SHA, "immutable exact nominal source forwarding continuation")
    record = read(NOMINAL)
    validate_nominal_source(record)
    return record["paths"]


def validate_continuation(record):
    from generation_decision_source_admission_5_0_6 import current_input_matches as decision_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {CUSTOM, DATABASE}, "one current custom reader and one exact compiler-database edge")
    for name, row in record["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha((ROOT / name).read_bytes())
                     or decision_matches(ROOT / name, row["after_sha256"])),
                "exact reader/compiler-database preimage and current bytes")
    row = record["paths"][CUSTOM]
    require(row["before_sha256"] == initial()["inputs_after"]["runtime:" + CUSTOM], "previously installed custom reader")
    trees = [ast.parse(row[key]) for key in ("before_source", "after_source")]
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)} for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and [name for name in functions[0] if functions[0][name] != functions[1][name]] == ["validate", "main"],
            "only explicit new-registration admission and accurate reporting change")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in ("validate", "main"):
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "all historical custom comparison controls remain exact")
    row = record["paths"][DATABASE]
    require(sha(SQL.read_bytes()) == SQL_SHA
            and read(SQL)["input_hashes_after"][str(ROOT / DATABASE)] == row["before_sha256"],
            "actual frozen constant-function compilation database")
    source = row["after_source"]
    marker = source.index("/tests/parity/narrow_integer_codec_probe.cpp")
    start = source.rfind("{", 0, marker)
    entry, end = json.JSONDecoder().raw_decode(source, start)
    require(entry["file"] == str(ROOT / "tests/parity/narrow_integer_codec_probe.cpp")
            and source[end:end + 2] == ",\n"
            and source[:start] + source[end + 2:] == row["before_source"],
            "removing only the newly registered observer recovers every preceding compiler-database byte")
    before, after = json.loads(row["before_source"]), json.loads(source)
    require(len(before) == 825 and len(after) == 826
            and sum(item["file"] == entry["file"] for item in after) == 1,
            "825 identical historical compilation commands and one new observer")


@cache
def continuation_sources():
    require(sha(CONTINUATION.read_bytes()) == CONTINUATION_SHA, "immutable custom-reader/compiler-database continuation")
    record = read(CONTINUATION)
    validate_continuation(record)
    return record["paths"]


def separate_custom_registration(entries, build):
    """Validate the new source binding before invoking the preserved 160-row join."""
    sources()
    continuation_sources()
    by_name = {row["name"]: row for row in entries}
    require(len(by_name) == len(entries), "unique current custom comparison registry")
    comparison = by_name["narrow_integer_codec_rust_parity"]
    fixture = by_name["narrow_integer_codec_source_contracts"]
    python = fixture["command"][0]
    require(fixture["command"] == [python, "-B", str(ROOT / SOURCE_FIXTURE), "--check"]
            and comparison["command"] == [python, str(ROOT / "tests/parity/compare_narrow_integer_codec_5_0_6.py"),
                                           "--cpp", str(Path(build) / "teaql_narrow_integer_codec_probe"), "--cpp-emulator="],
            "exact new comparison and complete-source fixture commands")
    props = lambda row: {item["name"]: item["value"] for item in row["properties"]}
    require(props(fixture).get("FIXTURES_SETUP") == ["narrow_integer_codec"]
            and props(fixture).get("RUN_SERIAL") is True and props(fixture).get("TIMEOUT") == 180
            and props(comparison).get("FIXTURES_REQUIRED") == ["narrow_integer_codec"]
            and props(comparison).get("DEPENDS") == ["narrow_integer_codec_source_contracts"]
            and [row["name"] for row in entries if "narrow_integer_codec" in props(row).get("FIXTURES_SETUP", [])]
                == ["narrow_integer_codec_source_contracts"], "unique required complete-source prerequisite")
    for name in (SOURCE_FIXTURE, "tests/parity/compare_narrow_integer_codec_5_0_6.py",
                 "tests/parity/narrow_integer_codec_probe.cpp", "tests/parity/narrow_integer_codec_5_0_6.rs",
                 "teaql_runtime/include/teaql/runtime/generated_scalar_codec.hpp"):
        require(sha((ROOT / name).read_bytes()) == initial()["inputs_after"]["runtime:" + name], "unchanged qualified narrow source: " + name)
    sys.path.insert(0, str(ROOT / "tests/parity"))
    from compare_narrow_integer_codec_5_0_6 import corpus
    from compare_values import same
    corpus_value = corpus()
    execution_path = EVIDENCE / "narrow_integer_codec_current_5_0_6.json.gz"
    execution = read(execution_path)
    require(sha(execution_path.read_bytes()) == corpus_value["execution_receipt_sha256"]
            and execution["complete"] is True and execution["diagnosis"] is False and execution["mismatches"] == []
            and same(execution["cases"], [row["input"] for row in corpus_value["cases"]])
            and same(execution["original"], [row["expected"] for row in corpus_value["cases"]])
            and same(execution["original"], execution["cpp"]), "all 376 corpus members bind to the actual complete original execution")
    row = dict(test="narrow_integer_codec_rust_parity", cohort="narrow_integer_codec",
               script="tests/parity/compare_narrow_integer_codec_5_0_6.py", source_test=fixture["name"],
               corpus="tests/parity/narrow_integer_codec_5_0_6_cases.json.gz", cases=376,
               corpus_sha256=sha((ROOT / "tests/parity/narrow_integer_codec_5_0_6_cases.json.gz").read_bytes()),
               execution_receipt="docs/parity/narrow_integer_codec_current_5_0_6.json.gz",
               execution_receipt_sha256=corpus_value["execution_receipt_sha256"], required_fixtures=["narrow_integer_codec"])
    return [item for item in entries if item["name"] != comparison["name"]], row


def added_input_hashes():
    require("runtime:" + SELF not in initial()["inputs_after"], "new registration identity helper")
    require("runtime:" + DRIVER not in initial()["inputs_after"], "new registration execution/receipt driver")
    reader_sources()
    return {"runtime:" + SELF: sha(Path(__file__).read_bytes()),
            "runtime:" + DRIVER: sha((ROOT / DRIVER).read_bytes())}


def controls():
    sources()
    result = []
    mutations = (
        ("missing_input", lambda row: row["paths"].pop("tests/CMakeLists.txt")),
        ("wrong_preimage", lambda row: row["paths"]["tests/CMakeLists.txt"].update(before_sha256="0" * 64)),
        ("forged_manifest", lambda row: row["paths"]["modules/source_contracts.json"].update(after_source="{}\n")),
        ("changed_identity_guard", lambda row: row["paths"][next(iter(GUARDS))].update(after_source="pass\n")),
        ("missing_driver", lambda row: row["paths"].pop(DRIVER)),
        ("false_complete", lambda row: row.update(complete=False)),
    )
    for name, mutate in mutations:
        record = copy.deepcopy(read(SOURCE))
        mutate(record)
        try:
            validate_sources(record)
        except (RuntimeError, KeyError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted registration source corruption: " + name)
    from compiler_offline_source_admission_5_0_6 import frame_matches, registered_epoch_matches
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    current = execution_inputs()
    require(frame_matches(current, initial()["inputs_after"]), "exact current registration source/addition epoch")
    require(not frame_matches(dict(current, **{"runtime:unknown_registration.cpp": "0" * 64}), initial()["inputs_after"])
            and not registered_epoch_matches(ROOT / "tests/CMakeLists.txt", "0" * 64),
            "unknown sources and forged historical hashes reject")
    result.append(dict(case="unknown_or_forged_source", rejected=True))
    for name, mutate in (
        ("missing_reader_correction", lambda row: row["paths"].pop(DRIVER)),
        ("forged_reader_image", lambda row: row["paths"][DRIVER].update(after_sha256="0" * 64)),
        ("wrong_executed_helper", lambda row: row.update(helper_before_source="pass\n")),
        ("wrong_current_helper", lambda row: row.update(helper_after_source_normalized="pass\n")),
        ("incomplete_reader_correction", lambda row: row.update(complete=False)),
    ):
        record = copy.deepcopy(read(READER))
        mutate(record)
        try:
            validate_reader_correction(record)
        except (RuntimeError, KeyError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted result-reader source corruption: " + name)
    for name, mutate in (
        ("missing_compiler_database", lambda row: row["paths"].pop(DATABASE)),
        ("forged_compiler_preimage", lambda row: row["paths"][DATABASE].update(before_sha256="0" * 64)),
        ("changed_prior_compile_command", lambda row: row["paths"][DATABASE].update(before_source="[]\n")),
        ("forged_custom_reader", lambda row: row["paths"][CUSTOM].update(after_source="pass\n")),
        ("extra_reader_source", lambda row: row["paths"].update({"unknown.py": {}})),
    ):
        record = copy.deepcopy(read(CONTINUATION))
        mutate(record)
        try:
            validate_continuation(record)
        except (RuntimeError, KeyError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler-database/custom-reader corruption: " + name)
    for name, mutate in (
        ("missing_nominal_source", lambda row: row["paths"].pop(NOMINAL_HELPER)),
        ("forged_nominal_preimage", lambda row: row["paths"][NOMINAL_HELPER].update(before_sha256="0" * 64)),
        ("changed_nominal_body", lambda row: row["paths"][NOMINAL_HELPER].update(after_source="pass\n")),
    ):
        record = copy.deepcopy(read(NOMINAL))
        mutate(record)
        try:
            validate_nominal_source(record)
        except (RuntimeError, KeyError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted nominal-source corruption: " + name)
    import subprocess
    entries = json.loads(subprocess.check_output(
        ["ctest", "--preset", "native", "--show-only=json-v1"], cwd=ROOT, text=True))["tests"]
    separate_custom_registration(entries, ROOT / "build_native_current")
    for name, mutate in (
        ("missing_new_comparison", lambda rows: rows.__setitem__(slice(None), [row for row in rows if row["name"] != "narrow_integer_codec_rust_parity"])),
        ("duplicate_new_comparison", lambda rows: rows.append(copy.deepcopy(next(row for row in rows if row["name"] == "narrow_integer_codec_rust_parity")))),
        ("missing_new_fixture", lambda rows: rows.__setitem__(slice(None), [row for row in rows if row["name"] != "narrow_integer_codec_source_contracts"])),
        ("wrong_new_program", lambda rows: next(row for row in rows if row["name"] == "narrow_integer_codec_rust_parity")["command"].__setitem__(3, "/unqualified/program")),
        ("missing_new_requirement", lambda rows: next(row for row in rows if row["name"] == "narrow_integer_codec_rust_parity").update(properties=[])),
        ("wrong_source_admission", lambda rows: next(row for row in rows if row["name"] == "narrow_integer_codec_source_contracts")["command"].__setitem__(-1, "--forged")),
        ("lost_source_setup", lambda rows: next(row for row in rows if row["name"] == "narrow_integer_codec_source_contracts").update(properties=[])),
    ):
        changed = copy.deepcopy(entries)
        mutate(changed)
        try:
            separate_custom_registration(changed, ROOT / "build_native_current")
        except (RuntimeError, KeyError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted new custom-registration corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    sources()
    continuation_sources()
    nominal_sources()
    reader_sources()
    if args.controls:
        require(not args.controls.exists(), "preserve registration source controls")
        write(args.controls, dict(complete=True, whole_goal_complete=False,
                                 optimized=sys.flags.optimize, controls=controls()))
    print("Verified exact narrow-integer source-fixture transition; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
