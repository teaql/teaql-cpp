#!/usr/bin/env python3
"""Qualify dynamic C++ routes and exact historical source continuations."""

import argparse
import ast
import copy
from functools import cache
import json
from pathlib import Path
import re
import sys

from verify_compiler_generated_business_5_0_6 import read, require, sha, write

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "initial": ("compiler_dynamic_route_initial_5_0_6.json.gz", "aa2244893d5e1a410689ffa2a3ce7bf1e0c3744ef475e9953bc53b92fa335330"),
    "units": ("compiler_dynamic_route_units_repaired_5_0_6.json.gz", "a31447058e7d2fbc69e2423bc64c8e6e5162b3f6d235dff8355e0b5758210676"),
    "complete": ("compiler_dynamic_route_complete_5_0_6.json.gz", "5ef67f4197c96e39eb19003f0c27f0f5afa7dd699cc7405679e4dfb5003e71cd"),
    "partial": ("compiler_dynamic_route_partial_5_0_6.json.gz", "3c242424996ebf26d58351715244d16043878d70e9b90d5fd24902380c541122"),
    "complete_sinks": ("compiler_dynamic_route_complete_sinks_5_0_6.json.gz", "e525dee790c25d8d394239ca593ece3087e1c8ef06dc2fd5802ef374458bd059"),
    "partial_sinks": ("compiler_dynamic_route_partial_sinks_5_0_6.json.gz", "75fe5feb85d4c4ddef0ad026eba079a9c5b362936cf34c3c5aca430b5afc3bdd"),
    "refusal": ("compiler_dynamic_route_original_refusal_5_0_6.json.gz", "67d0a80ab502fab599f97b61576d92dd4614a77b99703b3d6f54fa7745b3b8a9"),
    "sources": ("compiler_dynamic_route_sources_5_0_6.json.gz", "21b53bc827faaf403678b0df07cdf77701127d5cf45daeaf0d664c3fc265f3ef"),
    "reader_continuation": ("compiler_dynamic_route_reader_continuation_5_0_6.json.gz", "3d6e56652b7a5aa847de05e1924374e2b25b2d332c2b0a98bf59468ac6858668"),
}
GLOBAL = "runtime:scripts/compiler_host_governance_source_admission_5_0_6.py"
LOCAL = "runtime:scripts/compiler_scalar_consumer_source_admission_5_0_6.py"
READERS = {
    "runtime:scripts/verify_compiler_host_assembly_5_0_6.py",
    "runtime:scripts/verify_compiler_host_governed_5_0_6.py",
    "runtime:scripts/verify_compiler_scalar_consumer_results_5_0_6.py",
}


@cache
def bound(key):
    name, digest = RECORDS[key]
    path = EVIDENCE / name
    require(sha(path.read_bytes()) == digest, "immutable dynamic route evidence: " + name)
    return read(path)


def source_path(name):
    from compiler_host_governance_source_admission_5_0_6 import source_path as path
    return path(name)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_reader_continuation(record):
    from compiler_offline_source_admission_5_0_6 import qualified_transition
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {LOCAL}
            and record["preceding_sources_sha256"] == RECORDS["sources"][1], "one exact added-reader identity continuation")
    row = record["paths"][LOCAL]
    require(row["before_source"] == bound("sources")["paths"][LOCAL]["after_source"]
            and row["before_sha256"] == sha(row["before_source"].encode())
            and row["after_sha256"] == sha(row["after_source"].encode())
            and (row["after_sha256"] == sha(source_path(LOCAL).read_bytes())
                 or qualified_transition(LOCAL, row["after_sha256"], sha(source_path(LOCAL).read_bytes()))),
            "exact preceding/current reader source images")
    old = "            and all(current[name] == additions[name] for name in current.keys() - historical.keys())\n"
    new = ("            and all(current[name] == additions[name]\n"
           "                    or current[name] == sha(continuation.source_path(name).read_bytes())\n"
           "                    and continuation.registered_epoch_matches(continuation.source_path(name), additions[name])\n"
           "                    for name in current.keys() - historical.keys())\n")
    require(row["before_source"].count(old) == 1 and row["before_source"].replace(old, new, 1) == row["after_source"],
            "only exact known added-reader epochs change; provided current bytes remain checked")
    failure = EVIDENCE / "compiler_dynamic_route_validation_5_0_6.json.gz"
    require(record["failure_sha256"] == sha(failure.read_bytes()), "retain the original failed compatibility command")


@cache
def reader_continuation():
    record = bound("reader_continuation")
    validate_reader_continuation(record)
    return record


def validate_sources(record):
    initial, units = bound("initial"), bound("units")
    expected = {"producer:" + name for name in initial["source_images"]} | READERS | {GLOBAL, LOCAL}
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == expected, "six producer and five existing reader transitions")
    for name, row in record["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(source_path(name).read_bytes())
                     or name == LOCAL and row["after_sha256"] == reader_continuation()["paths"][LOCAL]["before_sha256"]),
                "exact before/after source images")
        if name.startswith("producer:"):
            path = name.split(":", 1)[1]
            require(row["before_source"] == initial["source_images"][path]["source"]
                    and row["after_source"] == units["source_images"][path]["after_source"],
                    "actual preceding and newly compiled producer source")
        else:
            require(row["before_sha256"] == initial["inputs_after"][name], "actual preceding reader bytes")
            before, after = functions(row["before_source"]), functions(row["after_source"])
            changed = (["registered_epoch_matches", "current_input_matches", "frame_matches", "java_inventory_matches"]
                       if name == GLOBAL else ["validate_sources", "frame_matches"] if name == LOCAL else ["validate"])
            require(before.keys() == after.keys() and [key for key in before if before[key] != after[key]] == changed,
                    "only registered source guards change; execution/comparison/control functions stay exact")
            trees = [ast.parse(row[key]) for key in ("before_source", "after_source")]
            for tree in trees:
                for node in tree.body:
                    if isinstance(node, ast.FunctionDef) and node.name in changed:
                        node.body = [ast.Pass()]
            require(ast.dump(trees[0]) == ast.dump(trees[1]), "no other reader/module changes")
    row = record["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/TemplatePreviewService.java"]
    restored = row["after_source"].replace("import com.skynet.codegenerator.cpp.CppCoreGenerator;\n", "", 1)
    restored = restored.replace("                                    CppCoreGenerator.SCOPE,\n", "", 1)
    block = ("        // The C++ generator emits its target in code rather than from an ST directory.\n"
             "        if (CppCoreGenerator.SCOPE.equals(rootTarget)) {\n            return true;\n        }\n")
    require(restored.replace(block, "", 1) == row["before_source"], "production change is canonical code-target recognition only")


@cache
def sources():
    record = bound("sources")
    validate_sources(record)
    return record


@cache
def source_epochs():
    from compiler_host_governance_source_admission_5_0_6 import source_epochs as previous
    rows = {path: list(images) for path, images in previous().items()}
    for name, image in sources()["paths"].items():
        rows.setdefault(source_path(name), []).append(image)
    rows[source_path(LOCAL)].append(reader_continuation()["paths"][LOCAL])
    return rows


def registered_epoch_matches(path, expected):
    from compiler_offline_source_admission_5_0_6 import registered_epoch_matches as current
    return current(path, expected)


def current_input_matches(path, expected):
    from compiler_offline_source_admission_5_0_6 import current_input_matches as active
    return active(path, expected)


def frame_matches(current, historical):
    return all(name in current and (current[name] == value or registered_epoch_matches(source_path(name), value))
               for name, value in historical.items())


def java_inventory_matches(current, historical):
    from compiler_offline_source_admission_5_0_6 import java_inventory_matches as active
    return active(current, historical)


def validate_units(record):
    from verify_compiler_generated_business_5_0_6 import execution_inputs, references
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and record["references_before"] == record["references_after"] == references()
            and record["probe_source"] == bound("initial")["probe_source"], "stable complete current Java/controller execution")
    expected = {"current_tests_javac": 0, "current_host_contracts": 0, "current_all_contracts": 0,
                "current_preview_contracts": 0, "current_route_probe_javac": 0, "current_dynamic_routes": 0,
                "previous_target_javac": 0, "previous_host_contracts": 1, "previous_preview_contracts": 1}
    require({step["label"]: step["exit_code"] for step in record["steps"]} == expected
            and len(record["steps"]) == len(expected)
            and all(type(step["exit_code"]) is int for step in record["steps"]), "all actual current/previous Java commands")
    steps = {step["label"]: step for step in record["steps"]}
    for name, count in (("current_host_contracts", 10), ("current_all_contracts", 275), ("current_preview_contracts", 7)):
        require(re.search(r"OK \(" + str(count) + r" tests\)", steps[name]["stdout"]) is not None, "actual complete Java test count")
    require("Tests run: 10,  Failures: 3" in steps["previous_host_contracts"]["stdout"]
            and "Tests run: 7,  Failures: 1" in steps["previous_preview_contracts"]["stdout"], "exact preceding production class fails route regressions")
    require(len(record["observations"]) == 2 and all(row["completed"] is True
            and type(row["compiler_calls"]) is int and row["compiler_calls"] == 1
            and type(row["response_bytes"]) is int and row["response_bytes"] > 0 for row in record["observations"]), "actual multipart/raw callback and ZIP observations")
    require(all(registered_epoch_matches(Path(name), digest) for name, digest in record["prepared_java_sources"].items())
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "actual compiled bytes and cleanup")
    require(sha(json.dumps(record, sort_keys=True).encode()) == sha(json.dumps(bound("units"), sort_keys=True).encode()),
            "immutable complete Java execution image")


def validate_runtime(record, key):
    import verify_primitive_const_governed_execution_5_0_6 as engine
    from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, references
    from verify_compiler_model_configuration_5_0_6 import worker_module
    engine.validate(record, bound(key + "_sinks"))
    flag = "--dynamic-upload" if key == "complete" else "--dynamic-body"
    require(record["dynamic_route"] == flag and record["case"]["name"] == key
            and record["host_adapter_source"] == record["dynamic_adapter_source"]
            and record["dynamic_adapter_sha256"] == sha(record["dynamic_adapter_source"].encode())
            and record["dynamic_base_sha256"] == sha(record["dynamic_base_source"].encode()), "actual copied host adapter and route")
    restored = record["dynamic_adapter_source"]
    require(len(record["dynamic_source_changes"]) == 2, "only explicit ROOT and host-route changes")
    for change in reversed(record["dynamic_source_changes"]):
        require(restored.count(change["after"]) == 1, "one exact copied-adapter replacement")
        restored = restored.replace(change["after"], change["before"], 1)
    require(restored == record["dynamic_base_source"]
            == sources()["paths"]["runtime:scripts/verify_compiler_host_governed_5_0_6.py"]["before_source"], "every original host execution/comparison body remains exact")
    require(record["host_inputs_before"] == record["host_inputs_after"] and frame_matches(execution_inputs(), record["host_inputs_after"])
            and record["host_native_before"] == record["host_native_after"] == native_inputs()
            and record["host_references_before"] == record["host_references_after"] == references(), "complete source/native/reference binding")
    job = record["host_job"]
    worker = worker_module()
    require(job["complete"] is True and job["outcome"] == "resolved"
            and job["original_sources_before"] == job["original_sources_after"]
            and job["cpp_sources"] == record["cpp_generated_hashes"]
            and record["capabilities"]["schema"] == "teaql.rust_configuration.job.v4", "whole SDK and explicit semantic compiler capability")
    worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
    worker.validate_semantic_storage(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
    require(record["host_process"]["command"][-1] == flag
            and type(record["host_process"]["exit_code"]) is int and record["host_process"]["exit_code"] == 0
            and record["host_process"]["command"] == next(row["executed_command"] for row in record["process_adaptations"] if "CppCompilerHostProbe" in row["executed_command"]), "actual selected dynamic host process")
    require(record["host_service_jobs_remaining"] == record["host_compiler_jobs_remaining"] == []
            and record["host_directory_removed"] is True and record["dynamic_adapter_directory_removed"] is True
            and all(not Path(path).exists() for path in (record["host_directory"], record["dynamic_adapter_directory"], job["owned_directory"], job["publication_directory"])), "actual host/compiler/adapter cleanup")
    require(sha(json.dumps(record, sort_keys=True).encode()) == sha(json.dumps(bound(key), sort_keys=True).encode()), "immutable complete dynamic compiler/business image")


def validate_refusal(record):
    from verify_compiler_generated_business_5_0_6 import execution_inputs, references
    job = record["job"]
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and record["references_before"] == record["references_after"] == references(), "stable actual dynamic original refusal")
    require(job["complete"] is True and job["outcome"] == "compiler_refusal" and record["sdk_exists"] is False
            and any(error["code"] == "E0277" for error in job["compiler_errors"])
            and any(step["label"] == "original_compile" and type(step["exit_code"]) is int and step["exit_code"] == 101 for step in job["steps"]), "actual original trait-bound compiler failure with no SDK fallback")
    require(len(record["steps"]) == 2 and record["steps"][-1]["command"][-1] == "--dynamic-body"
            and type(record["steps"][-1]["exit_code"]) is int and record["steps"][-1]["exit_code"] == 2
            and record["service_jobs_remaining"] == record["compiler_jobs_remaining"] == []
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "actual refused dynamic process and cleanup")
    require(sha(json.dumps(record, sort_keys=True).encode()) == sha(json.dumps(bound("refusal"), sort_keys=True).encode()), "immutable actual original-refusal image")


@cache
def evidence():
    sources()
    validate_units(bound("units"))
    for key in ("complete", "partial"):
        validate_runtime(bound(key), key)
    validate_refusal(bound("refusal"))
    return True


def controls():
    evidence()
    results = []
    changes = (
        ("sources", "missing_source", lambda row: row["paths"].pop(GLOBAL)),
        ("sources", "forged_image", lambda row: row["paths"][LOCAL].update(after_source="pass\n")),
        ("reader_continuation", "unqualified_added_reader", lambda row: row["paths"][LOCAL].update(before_source="pass\n")),
        ("reader_continuation", "forged_current_reader", lambda row: row["paths"][LOCAL].update(after_sha256="0" * 64)),
        ("units", "unfinished_java", lambda row: row.update(complete=False)),
        ("units", "lost_java_command", lambda row: row["steps"].pop()),
        ("units", "boolean_callback_count", lambda row: row["observations"][0].update(compiler_calls=True)),
        ("complete", "wrong_route", lambda row: row.update(dynamic_route="--controller")),
        ("complete", "lost_process", lambda row: row["process_adaptations"].pop()),
        ("complete", "false_save_success", lambda row: row["cpp_save"]["graph_write_failure"].update(save_ok=True)),
        ("partial", "lost_whole_plan", lambda row: row["cpp_save"]["graph_policy_approved"].update(policy_operation_counts=[1])),
        ("partial", "wrong_sdk", lambda row: row["host_job"]["cpp_sources"].pop(next(iter(row["host_job"]["cpp_sources"])))),
        ("partial", "boolean_host_exit", lambda row: row["host_process"].update(exit_code=False)),
        ("refusal", "hidden_refusal", lambda row: row["job"].update(outcome="resolved")),
        ("refusal", "fallback_sdk", lambda row: row.update(sdk_exists=True)),
        ("refusal", "boolean_original_exit", lambda row: next(step for step in row["job"]["steps"] if step["label"] == "original_compile").update(exit_code=True)),
    )
    for key, name, mutate in changes:
        record = copy.deepcopy(bound(key))
        mutate(record)
        try:
            if key == "sources":
                validate_sources(record)
            elif key == "reader_continuation":
                validate_reader_continuation(record)
            elif key == "units":
                validate_units(record)
            elif key == "refusal":
                validate_refusal(record)
            else:
                validate_runtime(record, key)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            results.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted corrupted dynamic route evidence: " + name)
    require(not registered_epoch_matches(ROOT / "tests/CMakeLists.txt", "0" * 64)
            and not java_inventory_matches({"/unregistered/Java.java": "0" * 64}, {}), "unknown source/Java images reject")
    results.append(dict(case="unknown_source_inventory", rejected=True))
    import compiler_scalar_consumer_source_admission_5_0_6 as scalar
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    current = execution_inputs()
    reader = "runtime:scripts/verify_compiler_scalar_consumer_results_5_0_6.py"
    require(not scalar.frame_matches(dict(current, **{reader: "0" * 64}), scalar.bound("date")["inputs_after"]),
            "an admitted historical reader cannot authorize forged current input bytes")
    results.append(dict(case="forged_current_added_reader", rejected=True))
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve dynamic route corruption evidence")
        write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls()))
    print("Verified actual dynamic C++ compiler routes and source continuations; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
