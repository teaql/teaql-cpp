#!/usr/bin/env python3
"""Admit the checked narrow-integer decoder without weakening historical gates."""

import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "runtime": ("narrow_integer_codec_current_5_0_6.json.gz", "7d47f4653d167f8f60921b66c2d3346524b9ffb73c61d969ea66b16565bd7be4"),
    "diagnosis": ("narrow_integer_codec_diagnosis_current_5_0_6.json.gz", "da18ab3506c536c2a84607a1c4eb38f467809fc7d56ebba7c630888aeac9adf7"),
    "native": ("narrow_integer_codec_native_seed_5_0_6.json.gz", "88eec2fc3b883872811d7d1cd60cd220b58d7c2bd63bf10aa38fb3da46d17a6d"),
    "legacy": ("narrow_integer_codec_legacy_source_5_0_6.json.gz", "db3b2535b8ead7e362eecce8693ade21656fcb8c09fb262162c00c3bc2c836b9"),
    "source": ("narrow_integer_source_transition_5_0_6.json.gz", "db02512c9c8412ae6d40da5d5f32b17ce8e7faba507aaf06bac69c251c3b3c8c"),
    "readers": ("narrow_integer_reader_transition_5_0_6.json.gz", "c38373f22d1289cdd81575b1bdd77463f5904bc1915d5f451ed7749fb2b35682"),
    "reader_final": ("narrow_integer_reader_final_5_0_6.json.gz", "a48fd4f3032ad53df15f91b5ef7d19ae21fbeeae905e7310e6a211114dc32212"),
    "generator": ("compiler_integer_carrier_diagnosis_declared_5_0_6.json.gz", "55073026acee564eda2a3da4e304340fb116684f0c062d6e09e625414352c23e"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Narrow integer source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable actual receipt: " + name)
    return json.loads(gzip.decompress(data))


def source_path(name):
    repository, relative = name.split(":", 1)
    path = Path(relative)
    require(repository in ("runtime", "producer") and not path.is_absolute()
            and ".." not in path.parts, "repository-relative source identity")
    return (ROOT if repository == "runtime" else PRODUCER) / path


@cache
def source_epochs():
    from compiler_business_id_source_admission_5_0_6 import source_epochs as preceding_epochs
    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for key in ("source", "readers", "reader_final"):
        for name, row in bound(key)["paths"].items():
            result.setdefault(source_path(name), []).append(row)
    return result


def registered_epoch_matches(path, expected):
    path = Path(path)
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    observed = sha(path.read_bytes())
    pending, visited = [expected], set()
    while pending:
        candidate = pending.pop()
        if candidate == observed:
            return True
        if candidate in visited:
            continue
        visited.add(candidate)
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, [])
                       if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    path = Path(path)
    if not registered_epoch_matches(path, expected):
        return False
    if sha(path.read_bytes()) != expected:
        evidence()
    return True


def frame_matches(current, historical):
    return all(name in current and (current[name] == value
               or registered_epoch_matches(source_path(name), value))
               for name, value in historical.items())


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER / "generator/src/main/java")
                 and row.get("before_source", "existing") is None}
    if not unknown <= additions.keys():
        return False
    if unknown:
        evidence()
    return all(Path(name).is_file() and registered_epoch_matches(Path(name), additions[name]) for name in unknown)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_sources(records):
    source, readers, legacy = records["source"], records["readers"], records["legacy"]
    require(source["complete"] is True and source["whole_goal_complete"] is False
            and len(source["paths"]) == 6, "complete kernel, CMake and actual observer sources")
    for name, row in source["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact executed source image")
        require(row["before_source"] is None and row["before_sha256"] is None
                or row["before_source"] is not None and row["before_sha256"] == sha(row["before_source"].encode()), "exact source preimage")
        frame = records["native"]["inputs_after"] if name.endswith("CMakeLists.txt") or "compare_narrow" in name else records["runtime"]["inputs_after"]
        require(frame.get(name) == row["after_sha256"], "source used by actual complete comparison or installed CTest")
    header = source["paths"]["runtime:teaql_runtime/include/teaql/runtime/generated_scalar_codec.hpp"]
    require(legacy["complete"] is True and legacy["whole_goal_complete"] is False
            and legacy["legacy_byte_preservation"] is True
            and header["before_source"] == legacy["before_source"] == records["diagnosis"]["kernel_source"]
            and header["after_source"] == legacy["after_source"] == records["runtime"]["kernel_source"]
            and legacy["before_sha256"] == sha(legacy["before_source"].encode())
            and legacy["after_sha256"] == sha(legacy["after_source"].encode()), "exact before/after kernel evidence")
    restored = legacy["after_source"].replace(legacy["added_branch"], "", 1).replace(legacy["added_include"], "", 1)
    require(restored == legacy["before_source"] and legacy["added_include"] == "#include <utility>\n"
            and legacy["added_branch"].startswith("        } else if constexpr (std::same_as<T, std::int16_t>")
            and "std::in_range<T>" in legacy["added_branch"], "all legacy decoder bytes remain exact")
    cmake = source["paths"]["runtime:tests/CMakeLists.txt"]
    start = cmake["after_source"].index("    add_executable(teaql_narrow_integer_codec_probe ")
    end = cmake["after_source"].index("    add_executable(teaql_loaded_snapshot_probe ", start)
    require(cmake["after_source"][:start] + cmake["after_source"][end:] == cmake["before_source"], "only new native target/test registration")
    require(readers["complete"] is True and readers["whole_goal_complete"] is False
            and set(readers["paths"]) == {"runtime:scripts/compiler_business_id_source_admission_5_0_6.py",
                "runtime:scripts/verify_narrow_integer_codec_5_0_6.py"}, "two bounded reader continuations")
    for name, row in readers["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact reader images")
        before, after = functions(row["before_source"]), functions(row["after_source"])
        changes = [key for key in before if before[key] != after.get(key)]
        if name.endswith("verify_narrow_integer_codec_5_0_6.py"):
            require(changes == ["main"] and set(after) - set(before) == {"validate", "controls"}
                    and row["before_source"] == records["runtime"]["driver_source"], "unchanged actual commands, input generation and execution bodies")
        else:
            require(changes == ["registered_epoch_matches", "current_input_matches", "frame_matches", "java_inventory_matches"]
                    and before.keys() == after.keys(), "only source matching delegates; all prior controls remain exact")
    final = records["reader_final"]
    name = "runtime:scripts/verify_narrow_integer_codec_5_0_6.py"
    require(final["complete"] is True and final["whole_goal_complete"] is False
            and set(final["paths"]) == {name}, "one final dependency/linker reader repair")
    row = final["paths"][name]
    before, after = functions(row["before_source"]), functions(row["after_source"])
    require(row["before_source"] == readers["paths"][name]["after_source"]
            and row["before_sha256"] == sha(row["before_source"].encode())
            and row["after_sha256"] == sha(row["after_source"].encode())
            and registered_epoch_matches(source_path(name), row["after_sha256"])
            and before.keys() == after.keys()
            and [key for key in before if before[key] != after[key]] == ["validate", "controls"],
            "only reader checks and new corruptions change; execution remains exact")


def validate(records):
    from verify_narrow_integer_codec_5_0_6 import validate as validate_runtime
    from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, references
    from compare_values import same
    validate_sources(records)
    validate_runtime(records["runtime"])
    prior, current = records["diagnosis"], records["runtime"]
    require(prior["complete"] is True and prior["whole_goal_complete"] is False and prior["diagnosis"] is True
            and same(prior["cases"], current["cases"]) and same(prior["original"], current["original"])
            and len(prior["mismatches"]) == 272
            and sum(prior["original"][i]["ok"] for i in prior["mismatches"]) == 112,
            "actual counterexamples and unchanged original outcomes")
    require(prior["mismatches"] == [i for i, (left, right) in enumerate(zip(prior["original"], prior["cpp"], strict=True))
                                    if not same(left, right)], "exact complete counterexample indexes")
    require(prior["owned_directory_removed"] is True and not Path(prior["owned_directory"]).exists(), "diagnosis output cleanup")
    native = records["native"]
    require(native["complete"] is True and native["whole_goal_complete"] is False
            and native["owned_directory_removed"] is True and not Path(native["owned_directory"]).exists()
            and native["inputs_before"] == native["inputs_after"] and frame_matches(execution_inputs(), native["inputs_after"])
            and native["references_before"] == native["references_after"] == references()
            and native["native_before"] == native["native_after"] == native_inputs(), "actual installed CTest and stable source/native inputs")
    expected_command = ["ctest", "--preset", "native", "-R", "^narrow_integer_codec_rust_parity$",
                        "--output-junit", str(Path(native["owned_directory"]) / "junit.xml")]
    require(len(native["steps"]) == 1 and native["steps"][0]["command"] == expected_command
            and type(native["steps"][0]["exit_code"]) is int
            and native["steps"][0]["exit_code"] == 0, "actual successful CTest invocation")
    cases = list(ET.fromstring(native["junit"]).iter("testcase"))
    require([row.attrib["name"] for row in cases] == ["narrow_integer_codec_rust_parity"]
            and not any(row.find(tag) is not None for row in cases for tag in ("failure", "error", "skipped")), "exact successful installed test membership")
    generator = records["generator"]
    require(generator["complete"] is True and generator["semantic_acceptance"] is False
            and len(generator["cases"]) == 6, "complete generator diagnosis remains separate")
    for row in generator["cases"]:
        job = row["job"]
        require(row["original_evaluation"] == row["cpp_evaluation"] and job["outcome"] == "compiler_refusal"
                and job["compiler_errors"] and row["sdk_exists"] is False
                and job["owned_directory_removed"] is True and not Path(job["owned_directory"]).exists(), "actual original generator refusal; no new accepted configuration")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


def controls():
    records = evidence()
    mutations = (
        ("runtime", "incomplete", lambda r: r.update(complete=False)),
        ("runtime", "changed_value", lambda r: r["cpp"][0].update(value=999)),
        ("diagnosis", "missing_counterexample", lambda r: r["mismatches"].pop()),
        ("legacy", "changed_legacy_bytes", lambda r: r.update(before_source=r["before_source"] + "// forged\n")),
        ("source", "unexecuted_source", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("readers", "changed_control", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("reader_final", "unqualified_final_reader", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("native", "missing_native_test", lambda r: r.update(junit="<testsuite/>")),
        ("native", "boolean_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("native", "substituted_native_command", lambda r: r["steps"][0].update(command=["true"])),
        ("diagnosis", "duplicate_counterexample", lambda r: r["mismatches"].__setitem__(0, r["mismatches"][1])),
        ("generator", "hidden_reference_refusal", lambda r: r["cases"][0]["job"].update(outcome="resolved")),
    )
    result = []
    for key, name, mutate in mutations:
        changed = dict(records)
        changed[key] = copy.deepcopy(records[key])
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted narrow integer source corruption: " + name)
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set())
            and not registered_epoch_matches(ROOT / "tests/CMakeLists.txt", "0" * 64), "unknown source additions and hashes reject")
    result.append(dict(case="unknown_source", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve earlier controls")
        result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls())
        args.controls.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
        print(json.dumps(dict(complete=True, optimized=sys.flags.optimize, controls=len(result["controls"]))))
    else:
        print("Verified checked narrow integer source, execution and legacy preservation")


if __name__ == "__main__":
    main()
