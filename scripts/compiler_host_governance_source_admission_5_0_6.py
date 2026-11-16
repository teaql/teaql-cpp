#!/usr/bin/env python3
"""Admit actual Spring-hosted governance and explicit identity diagnostics."""

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

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "complete": ("compiler_host_governed_complete_5_0_6.json.gz", "a565138425d621188634ab86702ec92862e0432200cfdb6882014208ffb4c01e"),
    "partial": ("compiler_host_governed_partial_5_0_6.json.gz", "2e0a2ebf1ed78df1cf1cf784763ee9c549f6b7f120865241ea5273ec04f01ee0"),
    "complete_sinks": ("compiler_host_governed_complete_sinks_5_0_6.json.gz", "9f987ebd485009de2edb032a168ef36fa6bd6c64266ae86912b4380f81fe6486"),
    "partial_sinks": ("compiler_host_governed_partial_sinks_5_0_6.json.gz", "051e9f05b870501473d1a34716c0f6e632e62a562a5b4a0675f9a5bd58ae823b"),
    "identity": ("compiler_identity_boundaries_5_0_6.json.gz", "9c0874966d33e7e65ffa8245892f1b0257f42224e2298bd5a3899970dccfee7f"),
    "option": ("compiler_identity_option_5_0_6.json.gz", "d05267a175496a6f1f9bfcf0e2e15491437228eed0efd1eb9d5af109404757e2"),
    "native_identity": ("compiler_identity_native_continuation_5_0_6.json.gz", "86bc650b00dc740aa1d80eef7577074ac44c6ad2894519e2aaa82e36e182b6f4"),
    "source": ("compiler_host_governance_sources_5_0_6.json.gz", "2be874c860b3ca7d2c5aa28825bb45915eff0a82b0e589ffbcb81c00c9ca7a5d"),
    "continuation": ("compiler_identity_continuation_sources_5_0_6.json.gz", "38c6db86b02b49f775f48aaac9fbc0900763e1f4eb06f8841f673d61bffdfd34"),
    "readers": ("compiler_host_governance_readers_5_0_6.json.gz", "5aabf19e354f91be04494796878e7e6e3a3e9100e629a78de8bb1c9fa0a603d7"),
    "reader_repair": ("compiler_host_governance_reader_repair_5_0_6.json.gz", "83a4f6bd643ca8e5f9c7c10def89427711384e0a7c8d55fc2948f8f1f149515c"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Compiler host governance admission: " + message)


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
    require(repository in ("runtime", "producer") and not path.is_absolute() and ".." not in path.parts,
            "repository-relative source identity")
    return (ROOT if repository == "runtime" else PRODUCER) / path


@cache
def source_epochs():
    from compiler_host_source_admission_5_0_6 import source_epochs as preceding_epochs
    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for key in ("source", "continuation", "readers", "reader_repair"):
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
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, []) if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    path = Path(path)
    if not registered_epoch_matches(path, expected):
        return False
    if sha(path.read_bytes()) != expected:
        evidence()
    return True


def frame_matches(current, historical):
    return all(name in current and (current[name] == value or registered_epoch_matches(source_path(name), value))
               for name, value in historical.items())


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER) and path.suffix == ".java"
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
    for key in ("source", "continuation", "readers", "reader_repair"):
        receipt = records[key]
        require(receipt["complete"] is True and receipt["whole_goal_complete"] is False, "complete source transition")
        for name, row in receipt["paths"].items():
            require(row["after_sha256"] == sha(row["after_source"].encode())
                    and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact source image")
            require(row["before_source"] is None and row["before_sha256"] is None
                    or row["before_source"] is not None and row["before_sha256"] == sha(row["before_source"].encode()), "exact source preimage")
    source = records["source"]["paths"]
    gov = "runtime:scripts/verify_compiler_host_governed_5_0_6.py"
    diag = "runtime:scripts/diagnose_compiler_identity_boundaries_5_0_6.py"
    probe = "runtime:tests/parity/compiler_property_identity_probe.cpp"
    require(set(source) == {gov, diag, probe}
            and source[gov]["after_source"] == records["complete"]["host_adapter_source"] == records["partial"]["host_adapter_source"]
            and source[diag]["after_source"] == records["identity"]["driver_source"]
            and source[probe]["after_source"] == records["identity"]["observer_source"], "actual execution and initial diagnostic sources")
    continuation = records["continuation"]["paths"]
    require(set(continuation) == {diag, probe}
            and continuation[diag]["before_source"] == records["identity"]["driver_source"]
            and continuation[diag]["after_source"] == records["option"]["driver_source"]
            and continuation[probe]["before_source"] == records["native_identity"]["observer_before"]
            and continuation[probe]["after_source"] == records["native_identity"]["observer_after"]
            and continuation[probe]["before_source"].replace("properties.emplace_back(Json(",
                "properties.emplace_back(teaql::Json(", 1) == continuation[probe]["after_source"], "only observer qualification and remaining-case collection")
    before, after = functions(continuation[diag]["before_source"]), functions(continuation[diag]["after_source"])
    require(before.keys() == after.keys() and [key for key in before if before[key] != after[key]] == ["execute", "main"], "case selection changes only the collector entry/execution loop")
    readers = records["readers"]["paths"]
    require(set(readers) == {gov, "runtime:scripts/compiler_host_source_admission_5_0_6.py"}, "two bounded source/receipt readers")
    for name, row in readers.items():
        before, after = functions(row["before_source"]), functions(row["after_source"])
        changed = [key for key in before if before[key] != after.get(key)]
        if name == gov:
            require(changed == ["main"] and set(after) - set(before) == {"validate", "controls"}
                    and row["before_source"] == records["complete"]["host_adapter_source"], "unchanged actual governed execution and model bodies")
        else:
            require(changed == ["registered_epoch_matches", "current_input_matches", "frame_matches", "java_inventory_matches"]
                    and before.keys() == after.keys(), "only source matching delegates; prior controls stay exact")
    repair = records["reader_repair"]["paths"]
    require(set(repair) == {gov}, "one governed command-provenance repair")
    row = repair[gov]
    before, after = functions(row["before_source"]), functions(row["after_source"])
    require(row["before_source"] == readers[gov]["after_source"] and before.keys() == after.keys()
            and [key for key in before if before[key] != after[key]] == ["validate", "controls"],
            "only command checks and new corruption control change")


def validate_identity(records):
    from diagnose_compiler_identity_boundaries_5_0_6 import cases, observer
    from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, references
    from verify_compiler_projection_generation_5_0_6 import native_command
    initial, option, native = records["identity"], records["option"], records["native_identity"]
    for record in (initial, option, native):
        require(record["semantic_acceptance"] is False and record["whole_goal_complete"] is False
                and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
                and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
                and record["references_before"] == record["references_after"] == references()
                and record["native_before"] == record["native_after"] == native_inputs(), "explicit stable identity diagnostic and cleanup scope")
    require(initial["complete"] is False and len(initial["cases"]) == 3
            and initial["steps"][-1]["label"] == "ordinary_column_native_build"
            and initial["steps"][-1]["exit_code"] == 1
            and "use of undeclared identifier 'Json'" in initial["steps"][-1]["stderr"], "preserved initial observer error")
    require(option["complete"] is True and option["selected_case"] == "option_plain_self"
            and len(option["cases"]) == 1, "only previously unexecuted Option case is collected")
    combined = initial["cases"] + option["cases"]
    require([row["name"] for row in combined] == [row["name"] for row in cases()], "all four exact complete model boundaries")
    for row, expected in zip(combined, cases(), strict=True):
        require(all(row[key] == value for key, value in expected.items()), "unchanged pending/ordinary models")
        job = row["job"]
        require(job["complete"] is True and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and not Path(job["owned_directory"]).exists() and not Path(job["publication_directory"]).exists(), "all actual identity compiler jobs cleaned")
        if row["name"] == "ordinary_column":
            require(row["host_exit_code"] == 0 and job["outcome"] == "resolved" and row["sdk_exists"] is True
                    and row["original_sources"] == job["original_sources_before"] == job["original_sources_after"], "actual complete ordinary original SDK")
        else:
            require(type(row["host_exit_code"]) is int and row["host_exit_code"] == 2 and job["outcome"] == "compiler_refusal"
                    and job["compiler_errors"] and row["sdk_exists"] is False, "original whole-library refusal without SDK or exception")
            if row["name"].startswith("column_attribute"):
                require(len(job["compiler_errors"]) == 6 and all(message["code"] == "RUST_COMPILER_ERROR" for message in job["compiler_errors"]), "complete original syntax refusals, separate from entity-only observations")
    ordinary = combined[2]
    require(native["complete"] is True and native["initial_receipt_sha256"] == RECORDS["identity"][1]
            and native["original_execution_reused"] is True and native["cpp_sources"] == ordinary["cpp_sources"]
            and native["original_properties"] == ordinary["job"]["configuration"]
            and native["observer_source"] == observer(ordinary["namespace"], ordinary["job"]["original_inventory"]["entities"]), "complete original job and exact regenerated SDK reused explicitly")
    require(native["capabilities"] == bound("native_identity")["capabilities"], "exact actual native-continuation compiler capability")
    owned = Path(native["owned_directory"])
    base = native["capabilities"]["cpp_generator"]["command"]
    classpath = base[base.index("-cp") + 1]
    expected_commands = [
        [base[0], "-cp", classpath, "CppCompiledModel", str(owned / "model.json"), str(owned / "cpp"),
         "--rust-configuration=" + str(owned / "resolution.json"),
         "--rust-toolchain-lock=" + str(ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"), "--source-package"],
        list(map(str, native_command(owned / "cpp/include", owned / "observer.cpp", owned / "native"))),
        [str(owned / "native")],
    ]
    require([step["label"] for step in native["steps"]] == ["regenerate_bound_cpp", "native_build", "native_run"]
            and [step["command"] for step in native["steps"]] == expected_commands
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in native["steps"])
            and json.loads(native["steps"][-1]["stdout"]) == native["cpp_properties"], "actual native correction execution")
    expected = {(entity["name"], prop["name"]): prop for entity in ordinary["job"]["configuration"] for prop in entity["properties"]}
    rows = [prop for entity in native["cpp_properties"] for prop in entity]
    require(len(rows) == len(expected) == native["ordinary_property_comparisons"] == 12
            and {(row["entity"], row["name"]) for row in rows} == set(expected), "complete ordinary property membership")
    for row in rows:
        value = expected[(row["entity"], row["name"])]
        require(row["physical_column"] == row["json_column"] == value["column"]
                and row["max_length"] == value["max_length"], "actual physical/JSON/shape identity")


def validate(records):
    from verify_compiler_host_governed_5_0_6 import validate as validate_governed
    validate_sources(records)
    validate_identity(records)
    for name in ("complete", "partial"):
        validate_governed(records[name], records[name + "_sinks"])


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


def controls():
    records = evidence()
    mutations = (
        ("complete", "missing_save", lambda r: r["original_save"].pop("graph_success")),
        ("complete", "substituted_business_step", lambda r: r["steps"][10].update(command=["true"])),
        ("partial", "wrong_limit", lambda r: r["case"].update(limit="17")),
        ("complete_sinks", "missing_sink", lambda r: r["observations"].pop()),
        ("identity", "hidden_observer_failure", lambda r: r.update(complete=True)),
        ("identity", "hidden_original_refusal", lambda r: r["cases"][0]["job"].update(outcome="resolved")),
        ("option", "invented_sdk", lambda r: r["cases"][0].update(sdk_exists=True)),
        ("native_identity", "lost_property", lambda r: r["cpp_properties"][0].pop()),
        ("native_identity", "substituted_identity_command", lambda r: r["steps"][-1].update(command=["true"])),
        ("native_identity", "repeated_original_claim", lambda r: r.update(original_execution_reused=False)),
        ("source", "forged_source", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("continuation", "unqualified_correction", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("readers", "changed_execution", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
    )
    rows = []
    for key, name, mutate in mutations:
        changed = dict(records)
        changed[key] = copy.deepcopy(records[key])
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError, ValueError) as error:
            rows.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted host governance source corruption: " + name)
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set())
            and not registered_epoch_matches(ROOT / "tests/CMakeLists.txt", "0" * 64), "unknown sources reject")
    rows.append(dict(case="unknown_source", rejected=True))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve preceding source controls")
        result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls())
        args.controls.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
        print("Verified hosted governance and", len(result["controls"]), "source controls; optimized=", sys.flags.optimize)
    else:
        print("Verified hosted Save/publication and complete identity diagnostics")


if __name__ == "__main__":
    main()
