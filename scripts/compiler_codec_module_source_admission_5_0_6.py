#!/usr/bin/env python3
"""Admit the exact canonical narrow-codec module registration transition."""

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
INITIAL = EVIDENCE / "current_codec_module_initial_5_0_6.json.gz"
INITIAL_SHA = "86794bf8514355fea24c7e9adfef6a926267af7b4a1e2d383649d80da022f75c"
SOURCE = EVIDENCE / "current_codec_module_sources_5_0_6.json.gz"
SOURCE_SHA = "84e30a7f497a7c3094e3c612d39f88a0c669d4fdd15713becfd56ad7a3a79abc"
PREVIOUS = "scripts/compiler_offline_input_source_admission_5_0_6.py"
SELF = "scripts/compiler_codec_module_source_admission_5_0_6.py"
TARGET = "teaql_module_narrow_integer_codec_probe"
CONTRACT = dict(package="teaql_runtime", stem="narrow_integer_codec_probe",
                source="tests/parity/narrow_integer_codec_probe.cpp",
                import_module="teaql_runtime", cases=[])
REGISTRATION = '''if(TARGET teaql_module_narrow_integer_codec_probe)
    add_test(NAME module_narrow_integer_codec_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_narrow_integer_codec_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_narrow_integer_codec_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
endif()

'''


@cache
def initial():
    require(sha(INITIAL.read_bytes()) == INITIAL_SHA, "immutable initial module binding failure")
    return read(INITIAL)


def validate_sources(record):
    expected = {"CMakePresets.json", "modules/CMakeLists.txt", "modules/contracts.json",
                "modules/source_contracts.json", PREVIOUS}
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == expected, "four module metadata inputs and two identity guards")
    for name, row in record["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                == initial()["inputs_after"]["runtime:" + name]
                and row["after_sha256"] == sha(row["after_source"].encode())
                == sha((ROOT / name).read_bytes()), "exact recorded before/current module source images")
        if name != PREVIOUS:
            require(row["before_source"] == initial()["source_images"][name]["source"],
                    "actual preceding module metadata bytes")
    before = lambda name: json.loads(record["paths"][name]["before_source"])
    after = lambda name: json.loads(record["paths"][name]["after_source"])
    presets = before("CMakePresets.json")
    preset = next(row for row in presets["buildPresets"] if row["name"] == "modules_native")
    require(TARGET not in preset["targets"], "one previously missing module preset target")
    preset["targets"].append(TARGET)
    require(presets == after("CMakePresets.json"), "only the new canonical module executable enters the preset")
    contracts = before("modules/contracts.json")
    require(not any(row["stem"] == CONTRACT["stem"] for row in contracts["contracts"]), "new canonical source contract")
    contracts["contracts"].append(CONTRACT)
    require(contracts == after("modules/contracts.json"), "all preceding canonical contracts/fixtures remain exact")
    cmake = record["paths"]["modules/CMakeLists.txt"]
    require(cmake["after_source"].count(REGISTRATION) == 1
            and cmake["after_source"].replace(REGISTRATION, "", 1) == cmake["before_source"],
            "one exact existing-comparator module registration")
    bindings = before("modules/source_contracts.json")
    current = after("modules/source_contracts.json")
    bindings["native_registration_sha256"] = sha((ROOT / "tests/CMakeLists.txt").read_bytes())
    bindings["canonical_contracts_sha256"] = record["paths"]["modules/contracts.json"]["after_sha256"]
    name = "module_narrow_integer_codec_rust_parity"
    addition = dict(module=name, native="narrow_integer_codec_rust_parity",
                    command=["@PYTHON@", "@SOURCE@/tests/parity/compare_narrow_integer_codec_5_0_6.py",
                             "--cpp", "@BUILD@/modules/" + TARGET, "--cpp-emulator="],
                    properties=dict(DEPENDS=["module_source_contract_bindings"],
                                    FIXTURES_REQUIRED=["module_source_contract_bindings"],
                                    WORKING_DIRECTORY="@BUILD@/modules"))
    require(len(bindings["bindings"]) == 403 and not any(row["module"] == name for row in bindings["bindings"]),
            "all 403 historical module Python bindings")
    bindings["bindings"].append(addition)
    bindings["bindings"].sort(key=lambda row: row["module"])
    script = "tests/parity/compare_narrow_integer_codec_5_0_6.py"
    require(script not in bindings["source_programs"], "new comparator source binding")
    bindings["source_programs"][script] = sha((ROOT / script).read_bytes())
    require(bindings == current and len(current["fixtures"]) == 62,
            "only the exact new binding/program and current declaration identities change")
    row = record["paths"][PREVIOUS]
    trees = [ast.parse(row[key]) for key in ("before_source", "after_source")]
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)} for tree in trees]
    changed = ["source_rows", "added_input_hashes"]
    require(functions[0].keys() == functions[1].keys()
            and [name for name in functions[0] if functions[0][name] != functions[1][name]] == changed,
            "only two source identity/addition guards forward")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in changed:
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "all prior execution/comparison/control bodies remain exact")


@cache
def sources():
    require(sha(SOURCE.read_bytes()) == SOURCE_SHA, "immutable exact module source transition")
    record = read(SOURCE)
    validate_sources(record)
    return record["paths"]


def source_rows():
    return {"runtime:" + name: row for name, row in sources().items()}


def added_input_hashes():
    require("runtime:" + SELF not in initial()["inputs_after"], "one new module source admission helper")
    return {"runtime:" + SELF: sha(Path(__file__).read_bytes())}


def controls():
    sources()
    mutations = (
        ("missing_source", lambda row: row["paths"].pop("modules/contracts.json")),
        ("wrong_preimage", lambda row: row["paths"][PREVIOUS].update(before_sha256="0" * 64)),
        ("forged_registration", lambda row: row["paths"]["modules/CMakeLists.txt"].update(after_source="pass\n")),
        ("wrong_manifest", lambda row: row["paths"]["modules/source_contracts.json"].update(after_sha256="0" * 64)),
        ("wrong_preset", lambda row: row["paths"]["CMakePresets.json"].update(after_source="{}\n")),
    )
    result = []
    for name, mutate in mutations:
        record = copy.deepcopy(read(SOURCE))
        mutate(record)
        try:
            validate_sources(record)
        except (RuntimeError, ValueError, KeyError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted module source transition corruption: " + name)
    from compiler_offline_source_admission_5_0_6 import frame_matches, registered_epoch_matches
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    current = execution_inputs()
    require(frame_matches(current, initial()["inputs_after"]), "exact current source/addition frame")
    require(not frame_matches(dict(current, **{"runtime:unknown_module.cpp": "0" * 64}), initial()["inputs_after"])
            and not registered_epoch_matches(ROOT / "modules/CMakeLists.txt", "0" * 64),
            "unknown module sources and arbitrary historical identities reject")
    result.append(dict(case="unknown_or_forged_source_identity", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    sources()
    if args.controls:
        require(not args.controls.exists(), "preserve module source corruption evidence")
        write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                                 controls=controls()))
    print("Verified exact canonical module source transition; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
