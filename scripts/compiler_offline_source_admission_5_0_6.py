#!/usr/bin/env python3
"""Qualify offline compiler routing and exact retained source/artifact epochs."""

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
    "initial": ("compiler_offline_route_initial_5_0_6.json.gz", "9b0389f3ff3e1e14d3d30d1151570b5842444db9ea626fc1e78fb54c578e1e46"),
    "units": ("compiler_offline_route_units_5_0_6.json.gz", "26ecd5140fe8ae2c193d19072cfdc905088c3a7211cdfc9a2c22817518fa4955"),
    "sdks": ("compiler_offline_route_sdks_5_0_6.json.gz", "805942cafa3f2c5d26c0536c223c3ca1c0cd1169b06b24db787fda02dbe3af16"),
    "defaults": ("compiler_offline_route_default_sdks_5_0_6.json.gz", "dd81b5b30009cd4dc82816edd8abc54cfa3034c35d03a55072dc89a56fd02c92"),
    "business": ("compiler_offline_route_complete_5_0_6.json.gz", "8fd47d9de30ac1992dbe511cfa5c638182dfa95c762092f39f265bc2de927b42"),
    "sinks": ("compiler_offline_route_complete_sinks_5_0_6.json.gz", "b4570a6006119e0aa716416df449e65c4f1ec18b05c21174577b5b73500adfc9"),
    "sources": ("compiler_offline_route_sources_5_0_6.json.gz", "bd13730d0c2b3666c8c9b1611be23ab742f181cb421bb1d20a93e4939aaf94fa"),
    "reader_continuation": ("compiler_offline_route_reader_continuation_5_0_6.json.gz", "2914ad657a1c8327986f59ea88568570609d0319b7d2cbc93b029da84b62f127"),
}
CORE = "producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"
DYNAMIC = "runtime:scripts/compiler_dynamic_route_source_admission_5_0_6.py"
LOCAL = "runtime:scripts/compiler_scalar_consumer_source_admission_5_0_6.py"
SDK = "runtime:scripts/reproduce_offline_compiler_sdks_5_0_6.py"
SELF = "runtime:scripts/compiler_offline_source_admission_5_0_6.py"


@cache
def bound(key):
    name, digest = RECORDS[key]
    path = EVIDENCE / name
    require(sha(path.read_bytes()) == digest, "immutable offline compiler receipt: " + name)
    return read(path)


def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("runtime", "producer") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical project input identity")
    return (ROOT if scope == "runtime" else PRODUCER) / relative


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_reader_continuation(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {DYNAMIC} and record["preceding_sources_sha256"] == RECORDS["sources"][1],
            "one bounded recursive identity-guard repair")
    row = record["paths"][DYNAMIC]
    require(row["before_source"] == bound("sources")["paths"][DYNAMIC]["after_source"]
            and row["before_sha256"] == sha(row["before_source"].encode())
            and row["after_sha256"] == sha(row["after_source"].encode()) == sha(source_path(DYNAMIC).read_bytes()),
            "exact recorded guard source images")
    before, after = functions(row["before_source"]), functions(row["after_source"])
    require(before.keys() == after.keys() and [name for name in before if before[name] != after[name]] == ["current_input_matches"],
            "only the recursive source check forwards; execution/comparison/control functions stay exact")
    old = ("def current_input_matches(path, expected):\n    path = Path(path)\n"
           "    if not registered_epoch_matches(path, expected):\n        return False\n"
           "    if sha(path.read_bytes()) != expected:\n        evidence()\n    return True\n")
    new = ("def current_input_matches(path, expected):\n"
           "    from compiler_offline_source_admission_5_0_6 import current_input_matches as active\n"
           "    return active(path, expected)\n")
    require(row["before_source"].count(old) == 1 and row["before_source"].replace(old, new, 1) == row["after_source"],
            "one exact identity-only forwarding change")
    failed = EVIDENCE / "compiler_offline_route_validation_5_0_6.json.gz"
    require(record["failed_validation_sha256"] == sha(failed.read_bytes()), "original recursion failure stays immutable")


@cache
def reader_continuation():
    value = bound("reader_continuation")
    validate_reader_continuation(value)
    return value["paths"][DYNAMIC]


def validate_sources(record):
    from compiler_offline_input_source_admission_5_0_6 import qualified_transition as input_transition
    initial, units = bound("initial"), bound("units")
    expected = {"producer:" + name for name in units["source_images"]} | {DYNAMIC, LOCAL, SDK}
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == expected and len(expected) == 8, "five producer and three source-guard/SDK inputs")
    for name, row in record["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(source_path(name).read_bytes())
                     or name == DYNAMIC and row["after_sha256"] == reader_continuation()["before_sha256"]
                     or name == CORE and input_transition(name, row["after_sha256"], sha(source_path(name).read_bytes()))),
                "exact current offline source image")
        if row["before_source"] is None:
            require(row["before_sha256"] is None and name not in initial["inputs_after"], "actual new input")
        else:
            require(row["before_sha256"] == sha(row["before_source"].encode()) == initial["inputs_after"][name],
                    "actual preceding source image")
        if name.startswith("producer:"):
            require(row == units["source_images"][name.split(":", 1)[1]], "exact freshly compiled producer source")
        elif name == SDK:
            require(row["before_source"] is None and row["after_source"] == bound("sdks")["driver_source"],
                    "actual unchanged six-action SDK adapter")
        else:
            before, after = functions(row["before_source"]), functions(row["after_source"])
            changed = (["validate_reader_continuation", "registered_epoch_matches", "java_inventory_matches"]
                       if name == DYNAMIC else ["frame_matches"])
            require(before.keys() == after.keys() and [key for key in before if before[key] != after[key]] == changed,
                    "only exact historical identity/inventory guards change")
            trees = [ast.parse(row[key]) for key in ("before_source", "after_source")]
            for tree in trees:
                for node in tree.body:
                    if isinstance(node, ast.FunctionDef) and node.name in changed:
                        node.body = [ast.Pass()]
            require(ast.dump(trees[0]) == ast.dump(trees[1]), "execution, comparison, control and module bodies remain exact")
    row = record["paths"][CORE]
    old = ("    BaseDomainManager manager = new BaseDomainManager();\n"
           "    Domain domain = manager.getDomain(manager.parse(context));\n"
           "    List<GeneratedFile> generated = configuration != null\n"
           "        ? CppCompilerModelConfiguration.generator(configuration, modelBytes, compilerLock, domain).generate(domain)\n"
           "        : args.length > 2\n"
           "        ? new CppCoreGenerator(flags.contains(\"--qualify-inheritance\"), flags.contains(\"--qualify-constants\")).generate(domain)\n"
           "        : new DomainGenerationService().generateDomainFiles(domain, SCOPE);\n")
    new = ("    List<GeneratedFile> generated;\n"
           "    if (args.length == 2) {\n"
           "      generated = CppCompilerServiceConfiguration.fromSystemProperties().generateDomainFiles(context);\n"
           "    } else {\n"
           "      BaseDomainManager manager = new BaseDomainManager();\n"
           "      Domain domain = manager.getDomain(manager.parse(context));\n"
           "      generated = configuration != null\n"
           "          ? CppCompilerModelConfiguration.generator(configuration, modelBytes, compilerLock, domain).generate(domain)\n"
           "          : new CppCoreGenerator(flags.contains(\"--qualify-inheritance\"), flags.contains(\"--qualify-constants\")).generate(domain);\n"
           "    }\n")
    require(row["after_source"].count(new) == 1, "one exact offline main branch")
    restored = row["after_source"].replace(new, old, 1).replace(
        "import com.skynet.codegenerator.AbstractGenerator;\n",
        "import com.skynet.codegenerator.AbstractGenerator;\nimport com.skynet.codegenerator.DomainGenerationService;\n", 1)
    require(restored == row["before_source"], "every entity/query/expression emitter byte remains unchanged")


@cache
def sources():
    value = bound("sources")
    validate_sources(value)
    return value["paths"]


def qualified_transition(name, before, after):
    row = sources().get(name)
    return row is not None and row["before_sha256"] == before and row["after_sha256"] == after


@cache
def manifest_rows():
    from compiler_offline_input_source_admission_5_0_6 import qualified_transition as input_transition
    result = {}
    record = bound("sdks")
    require(record["complete"] is True and len(record["before"]) == len(record["after"]) == 80,
            "all eighty original/current SDKs")
    for before, after in zip(record["before"], record["after"], strict=True):
        name = before["manifest"]
        leaf = Path(name).name
        require(name == after["manifest"] and before["files"].keys() == after["files"].keys()
                and all(after["files"][key] == value for key, value in before["files"].items() if key != leaf),
                "all code and Assist bytes remain exact")
        expected = copy.deepcopy(before["value"])
        core = CORE.split(":", 1)[1]
        require(expected["generator_implementation_sha256"][core] == sources()[CORE]["before_sha256"], "exact previous emitter identity")
        expected["generator_implementation_sha256"][core] = sources()[CORE]["after_sha256"]
        require(expected == after["value"] and sha(before["text"].encode()) == before["files"][leaf]
                and sha(after["text"].encode()) == after["files"][leaf]
                and (after["files"][leaf] == sha((ROOT / name).read_bytes())
                     or input_transition("runtime:" + name, after["files"][leaf], sha((ROOT / name).read_bytes()))),
                "only the reviewed main-source fingerprint changes")
        result["runtime:" + name] = dict(before_source=before["text"], before_sha256=before["files"][leaf],
                                        after_source=after["text"], after_sha256=after["files"][leaf])
    return result


@cache
def source_epochs():
    from compiler_dynamic_route_source_admission_5_0_6 import source_epochs as previous
    result = {path.resolve(): list(rows) for path, rows in previous().items()}
    for name, row in {**sources(), **manifest_rows()}.items():
        result.setdefault(source_path(name).resolve(), []).append(row)
    result[source_path(DYNAMIC).resolve()].append(reader_continuation())
    from compiler_offline_input_source_admission_5_0_6 import source_rows
    for name, row in source_rows().items():
        result.setdefault(source_path(name).resolve(), []).append(row)
    return result


def registered_epoch_matches(path, expected):
    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    observed = sha(path.read_bytes())
    pending, seen = [expected], set()
    while pending:
        candidate = pending.pop()
        if candidate == observed:
            return True
        if candidate in seen:
            continue
        seen.add(candidate)
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, []) if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    if not registered_epoch_matches(path, expected):
        return False
    # Epoch construction verifies the immutable Java/source receipts and every
    # retained SDK byte. Do not re-enter a caller's business validator here.
    if sha(Path(path).read_bytes()) != expected:
        sources()
        manifest_rows()
    return True


def added_input_hashes():
    result = {name: row["after_sha256"] for name, row in sources().items() if row["before_source"] is None}
    result[SELF] = sha(Path(__file__).read_bytes())
    from compiler_offline_input_source_admission_5_0_6 import added_input_hashes as inputs_added
    result.update(inputs_added())
    return result


def frame_matches(current, historical):
    additions = added_input_hashes()
    return (historical.keys() <= current.keys() and current.keys() - historical.keys() <= additions.keys()
            and all(current[name] == additions[name] for name in current.keys() - historical.keys())
            and all(current[name] == value or current[name] == sha(source_path(name).read_bytes())
                    and registered_epoch_matches(source_path(name), value) for name, value in historical.items()))


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER) and path.suffix == ".java" and row.get("before_source", "existing") is None}
    return unknown <= additions.keys() and all(Path(name).is_file()
        and registered_epoch_matches(Path(name), additions[name]) for name in unknown)


def immutable(record, key):
    require(sha(json.dumps(record, sort_keys=True).encode()) == sha(json.dumps(bound(key), sort_keys=True).encode()),
            "immutable complete actual " + key + " execution image")


def validate_units(record):
    from verify_compiler_generated_business_5_0_6 import execution_inputs, references
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and record["references_before"] == record["references_after"] == references(), "actual frozen CLI/host tests")
    steps = {row["label"]: row for row in record["steps"]}
    expected = {"current_tests_javac": 0, "current_offline_contracts": 0, "current_all_contracts": 0,
                "previous_main_javac": 0, "previous_offline_contracts": 1}
    require({key: row["exit_code"] for key, row in steps.items()} == expected and len(steps) == len(record["steps"])
            and all(type(row["exit_code"]) is int for row in record["steps"] + record["preparation_steps"]), "all exact terminal Java commands")
    require("OK (7 tests)" in steps["current_offline_contracts"]["stdout"]
            and "OK (282 tests)" in steps["current_all_contracts"]["stdout"]
            and "Tests run: 7,  Failures: 4" in steps["previous_offline_contracts"]["stdout"], "actual new/current/preceding regression counts")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and all(registered_epoch_matches(Path(name), digest) for name, digest in record["prepared_java_sources"].items()), "actual compiled Java sources and cleanup")
    immutable(record, "units")


def validate_defaults(record):
    from preserve_compiler_projection_contracts_5_0_6 import BATCH, cases
    from verify_compiler_generated_business_5_0_6 import execution_inputs, references
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and type(record["actual_cpp_compiler_processes"]) is int and record["actual_cpp_compiler_processes"] == 0
            and record["reused_cpp_compiler_outcomes"] == 406 and record["planned_cases"] == cases()
            and len(record["cases"]) == 406 and record["batch_source"] == BATCH, "complete actual default batch; explicit earlier compiler reuse")
    for actual, expected in zip(record["cases"], record["planned_cases"], strict=True):
        require(actual["id"] == expected["id"] and actual["match"] is True and actual["files"] == expected["files"], "every complete SDK file matches")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and frame_matches(execution_inputs(), {key: value for key, value in record["input_hashes_after"].items()
                                                  if key != "runtime:docs/cpp_generator_parity_ledger.json"})
            and record["references_before"] == record["references_after"] == references()
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "stable SDK batch sources and cleanup")
    require([row["label"] for row in record["steps"]] == ["current_javac", "complete_sdk_batch"]
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0 for row in record["steps"])
            and "CPP_SDK_COUNT 406" in record["steps"][-1]["stdout"], "actual compiler and complete batch stdout")
    immutable(record, "defaults")


def validate_sdks(record):
    import reproduce_offline_compiler_sdks_5_0_6 as sdks
    sdks.validate(record)
    immutable(record, "sdks")


def validate_business(record):
    import verify_primitive_const_governed_execution_5_0_6 as engine
    from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, references
    engine.validate(record, bound("sinks"))
    process = record["offline_process"]
    require(type(process["exit_code"]) is int and process["exit_code"] == 0
            and "com.skynet.codegenerator.cpp.CppCoreGenerator" in process["command"]
            and all("-D" + name + "=" + value in process["command"] for name, value in record["offline_properties"].items()), "actual configured default CLI process")
    require(record["offline_cpp_provenance"]["schema"] == "teaql.rust_configuration.model_generation.v3"
            and record["capabilities"]["schema"] == "teaql.rust_configuration.job.v4"
            and record["offline_cpp_sources"] == record["cpp_generated_hashes"]
            and record["offline_artifacts_after"] == record["capabilities"]["artifacts"], "actual whole SDK and explicit semantic compiler facts")
    refusal = record["offline_reference_refusal"]
    require(type(refusal["exit_code"]) is int and refusal["exit_code"] != 0 and refusal["sdk_exists"] is False
            and "REFERENCE_COMPILER_REFUSAL" in refusal["stderr"] and "E0277" in refusal["stderr"], "actual original refusal; no fallback SDK")
    require(record["offline_adapter_sha256"] == sha(record["offline_adapter_source"].encode())
            and record["offline_inputs_before"] == record["offline_inputs_after"]
            and frame_matches(execution_inputs(), record["offline_inputs_after"])
            and record["offline_native_before"] == record["offline_native_after"] == native_inputs()
            and record["offline_references_before"] == record["offline_references_after"] == references(), "exact actual execution source/native/reference frames")
    require(record["offline_directory_removed"] is True and not Path(record["offline_directory"]).exists()
            and record["offline_service_jobs_remaining"] == record["offline_compiler_jobs_remaining"] == [], "all compiler/service and observer jobs reclaimed")
    immutable(record, "business")


@cache
def evidence():
    sources()
    manifest_rows()
    validate_sdks(bound("sdks"))
    validate_units(bound("units"))
    validate_defaults(bound("defaults"))
    validate_business(bound("business"))
    return True


def controls():
    evidence()
    changes = (
        ("sources", "missing_source", lambda row: row["paths"].pop(CORE)),
        ("sources", "forged_main", lambda row: row["paths"][CORE].update(after_source="class Changed {}\n")),
        ("sources", "wrong_guard_preimage", lambda row: row["paths"][DYNAMIC].update(before_sha256="0" * 64)),
        ("reader_continuation", "forged_identity_forward", lambda row: row["paths"][DYNAMIC].update(after_source="pass\n")),
        ("reader_continuation", "unqualified_identity_preimage", lambda row: row["paths"][DYNAMIC].update(before_sha256="0" * 64)),
        ("units", "unfinished_java", lambda row: row.update(complete=False)),
        ("units", "boolean_java_exit", lambda row: row["steps"][0].update(exit_code=False)),
        ("units", "hidden_previous_regression", lambda row: row["steps"][-1].update(exit_code=0)),
        ("sdks", "missing_retained_sdk", lambda row: row["after"].pop()),
        ("sdks", "forged_retained_code", lambda row: row["after"][0]["files"].update(invented="0" * 64)),
        ("sdks", "forged_main_fingerprint", lambda row: row["after"][0]["value"]["generator_implementation_sha256"].update({CORE.split(":", 1)[1]: "0" * 64})),
        ("defaults", "missing_default_sdk", lambda row: row["cases"].pop()),
        ("defaults", "boolean_default_compilation", lambda row: row.update(actual_cpp_compiler_processes=False)),
        ("defaults", "invented_compilation", lambda row: row.update(actual_cpp_compiler_processes=406)),
        ("defaults", "wrong_default_files", lambda row: row["cases"][0]["files"].update(invented="0" * 64)),
        ("business", "boolean_cli_exit", lambda row: row["offline_process"].update(exit_code=False)),
        ("business", "lost_compiler_property", lambda row: row["offline_process"]["command"].remove(next(value for value in row["offline_process"]["command"] if value.startswith("-Dteaql.cpp.compiler.capabilities=")))),
        ("business", "older_implicit_capability", lambda row: row["offline_cpp_provenance"].update(schema="teaql.rust_configuration.model_generation.v1")),
        ("business", "wrong_whole_sdk", lambda row: row["offline_cpp_sources"].pop(next(iter(row["offline_cpp_sources"])))),
        ("business", "false_graph_success", lambda row: row["cpp_save"]["graph_write_failure"].update(save_ok=True)),
        ("business", "lost_whole_plan", lambda row: row["cpp_save"]["graph_policy_approved"].update(policy_operation_counts=[1])),
        ("business", "fallback_sdk", lambda row: row["offline_reference_refusal"].update(sdk_exists=True)),
        ("business", "hidden_original_refusal", lambda row: row["offline_reference_refusal"].update(stderr="")),
    )
    result = []
    for key, name, mutate in changes:
        row = copy.deepcopy(bound(key))
        mutate(row)
        try:
            {"sources": validate_sources, "reader_continuation": validate_reader_continuation,
             "units": validate_units, "sdks": validate_sdks, "defaults": validate_defaults,
             "business": validate_business}[key](row)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted offline compiler corruption: " + name)
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    current = execution_inputs()
    require(not frame_matches(dict(current, **{"runtime:unknown.cpp": "0" * 64}), bound("business")["offline_inputs_after"])
            and not frame_matches(dict(current, **{CORE: "0" * 64}), bound("business")["offline_inputs_after"])
            and not registered_epoch_matches(ROOT / "tests/CMakeLists.txt", "0" * 64)
            and not java_inventory_matches({"/unregistered/Java.java": "0" * 64}, {}), "unknown or forged current source images reject")
    result.append(dict(case="unknown_or_forged_inputs", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve offline compiler corruption evidence")
        write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls()))
    print("Verified actual default offline compiler routing and retained SDK/source epochs; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
