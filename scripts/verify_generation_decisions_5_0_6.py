#!/usr/bin/env python3
"""Qualify the approved compiler-entry contract and two independent exceptions."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
MANIFEST = ROOT / "docs/compatibility_exceptions_5_0_6.json"
LEGACY_SHA = "35c85185c42fe862c26c89aa8d42d127e6cc7d94ea33e2302e32b8d3612e5b01"
PREVIOUS = "5d80f441f112b290e4bb2e00a9edac38c8e1c97f"
SEED = ROOT / "docs/parity/compiler_offline_inputs_units_5_0_6.json.gz"
SEED_SHA = "1516444b1a38ea5806cd85a966cdf3ceb6698223913cabc0355a4e045451f675"
JAVA = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerServiceConfiguration.java"
TESTS = ["CppCompilerHostConfigurationTest", "CppOfflineCompilerConfigurationTest",
         "CppOfflineModelInputTest"]
CHANGED = [JAVA, *("generator/src/test/java/com/skynet/codegenerator/cpp/" + name + ".java"
                   for name in TESTS)]
EXCEPTIONS = {
    "option_model": {
        "id": "option_model_name_collision", "status": "approved", "approved_on": "2026-11-20",
        "documentation": "docs/option_model_compatibility_exception_5_0_6.md",
        "cpp_contract": "Preserve the modeled C++ Option entity and Self projection; do not emulate the locked Rust prelude-name collision.",
        "case_names": ["option_plain_self"],
        "evidence_sha256": {
            "docs/parity/ap02_plain_option_model_diagnostic_5_0_6.json.gz":
                "58e63896733d147bd89581098f736e80aa676b0f604684f4312f6cc74a80e9f9",
        },
    },
    "column_attribute": {
        "id": "unescaped_column_attribute_injection", "status": "approved", "approved_on": "2026-11-20",
        "documentation": "docs/field_attribute_compatibility_exception_5_0_6.md",
        "cpp_contract": "Reject unescaped quotes in original Rust column literals explicitly before publishing generated files.",
        "case_names": ["column_attribute_alias", "column_attribute_shape"],
        "evidence_sha256": {
            "tests/parity/field_attribute_models_5_0_6.json.gz":
                "3083bf99f3b0c94ec6929978781edd382645e633e17346dad38ed473c61ff2e0",
            "docs/parity/field_attribute_after_reverse_types_property_exception_5_0_6.json.gz":
                "ecd68ea8e45c7f423d1d4599e832e5ab51e72f18bed9555677d5f64dc54d0853",
            "docs/parity/field_attribute_after_reverse_types_valid_control_5_0_6.json.gz":
                "53de5a9673c5513e9b09e3afed6066949856b380601bbc543c7b1fd4edd645ee",
        },
    },
}


def require(condition, message):
    if not condition:
        raise ValueError("Generation decisions: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    data = Path(path).read_bytes()
    return json.loads(gzip.decompress(data) if str(path).endswith(".gz") else data)


def write(path, value):
    Path(path).write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def validate_exceptions(contract):
    legacy = copy.deepcopy(contract)
    for name, expected in EXCEPTIONS.items():
        require(legacy.pop(name, None) == expected, "exact independently approved scope: " + name)
        require((ROOT / expected["documentation"]).is_file(), "existing exception document")
        for name, digest in expected["evidence_sha256"].items():
            require(sha((ROOT / name).read_bytes()) == digest, "immutable diagnostic: " + name)
    require(sha(json.dumps(legacy, sort_keys=True, separators=(",", ":")).encode()) == LEGACY_SHA,
            "all prior exception contracts and evidence remain unchanged")
    require(contract["runtime_commit"] == read(ROOT / "upstream.lock.json")["commit"], "locked runtime")
    require(contract["generator_commit"] == read(ROOT / "docs/cpp_generator_parity_ledger.json")["generator_commit"],
            "locked generator")
    option = read(ROOT / next(iter(EXCEPTIONS["option_model"]["evidence_sha256"])))
    require(option["complete"] is True and option["parity_passed"] is False
            and option["differences"] == ["option_plain_self"], "preserved original Option mismatch")
    case, = option["cases"]
    require(case["name"] == "option_plain_self" and type(case["original_exit_code"]) is int
            and case["original_exit_code"] == 101 and type(case["cpp_exit_code"]) is int
            and case["cpp_exit_code"] == 0 and case["type"] == "Self"
            and case["expected_entity"] == "Option" and case["match"] is False,
            "actual original failure and correct C++ projection")
    steps = {step["label"]: step for step in option["steps"]}
    require("E0107" in steps["option_plain_self_original_compile"]["stderr"]
            and steps["option_plain_self_original_compile"]["exit_code"] == 101
            and steps["option_plain_self_cpp_compile"]["exit_code"] == 0,
            "actual retained compiler diagnostics")
    attribute = read(ROOT / "docs/parity/field_attribute_after_reverse_types_property_exception_5_0_6.json.gz")
    require(attribute["complete"] is True and attribute["diagnostic_cpp_refusal"] is True
            and attribute["semantic_accepted"] is False
            and [case["name"] for case in attribute["cases"]] == EXCEPTIONS["column_attribute"]["case_names"],
            "retained original/refusal diagnostic is not relabeled parity")
    for index, case in enumerate(attribute["cases"]):
        require(type(case["cpp_refusal"]["exit_code"]) is int and case["cpp_refusal"]["exit_code"] != 0
                and "Unescaped quote in original Rust column literal" in case["cpp_refusal"]["stderr"],
                "exact C++ refusal")
        for entity in case["original"][1:]:
            field, = [field for field in entity["properties"] if field["name"] == "name"]
            require(field["column"] == "chosen" and type(field["max_length"]) is int
                    and field["max_length"] == (100 if index == 0 else 7), "executed injected descriptors")


def exception_controls(contract):
    variants = [
        ("old_exception_changed", lambda x: x["invalid_wire_naming"].update(status="pending")),
        ("reference_changed", lambda x: x.update(runtime_commit="0" * 40)),
        ("option_unapproved", lambda x: x["option_model"].update(status="pending")),
        ("option_scope_widened", lambda x: x["option_model"]["case_names"].append("all_types")),
        ("option_evidence_substituted", lambda x: x["option_model"].update(evidence_sha256={})),
        ("column_unapproved", lambda x: x["column_attribute"].update(status="pending")),
        ("column_scope_widened", lambda x: x["column_attribute"]["case_names"].append("all_columns")),
        ("column_refusal_changed", lambda x: x["column_attribute"].update(cpp_contract="ignore invalid input")),
        ("column_evidence_changed", lambda x: x["column_attribute"]["evidence_sha256"].update(
            {next(iter(x["column_attribute"]["evidence_sha256"])): "0" * 64})),
        ("approval_date_changed", lambda x: x["option_model"].update(approved_on="2026-10-24")),
    ]
    results = []
    for name, mutation in variants:
        changed = copy.deepcopy(contract)
        mutation(changed)
        try:
            validate_exceptions(changed)
        except (ValueError, KeyError, TypeError) as error:
            results.append(dict(name=name, rejected=True, error=str(error)))
        else:
            raise ValueError("Invalid approval admitted: " + name)
    return results


def qualify_java():
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    from verify_primitive_const_governed_execution_5_0_6 import run_owned

    require(sha(SEED.read_bytes()) == SEED_SHA, "immutable Java command recipe")
    seed = read(SEED)
    record = dict(schema="teaql.approved_generation_decisions.java.v1", complete=False,
                  whole_goal_complete=False, steps=[], source_images={}, seed_sha256=SEED_SHA,
                  inputs_before=execution_inputs())
    for name in CHANGED:
        before = subprocess.check_output(["git", "-C", str(PRODUCER), "show", PREVIOUS + ":" + name], text=True)
        after = (PRODUCER / name).read_text()
        record["source_images"][name] = dict(before_source=before, before_sha256=sha(before.encode()),
                                             after_source=after, after_sha256=sha(after.encode()))
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_approved_generation_java_506_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = str(owned)
            for name in ("cpp_classes", "java_tmp", "previous_classes"):
                (owned / name).mkdir()

            def run(label, command, expected=0):
                result = run_owned(command, cwd=PRODUCER / "generator", text=True,
                                   capture_output=True, timeout=900)
                record["steps"].append(dict(label=label, command=command, exit_code=result.returncode,
                                             stdout=result.stdout, stderr=result.stderr))
                print(label, "exit", result.returncode, flush=True)
                require(result.returncode == expected, label + ": " + result.stdout[-3000:] + result.stderr[-3000:])
                return result

            old = seed["owned_directory"]
            command = [part.replace(old, temporary) for part in seed["steps"][0]["command"]]
            for path in sorted((PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp").glob("*Test.java")):
                if str(path) not in command:
                    command.append(str(path))
            run("current_fresh_javac", command)
            all_command = [part.replace(old, temporary) for part in next(
                step["command"] for step in seed["steps"] if step["label"] == "current_all_contracts")]
            current = run("current_all_contracts", all_command)
            match = re.search(r"OK \((\d+) tests\)", current.stdout)
            require(match is not None, "actual successful JUnit test count")
            record["java_test_count"] = int(match[1])
            record["test_classes"] = all_command[all_command.index("org.junit.runner.JUnitCore") + 1:]
            previous = owned / "CppCompilerServiceConfiguration.java"
            previous.write_text(record["source_images"][JAVA]["before_source"])
            classpath = all_command[all_command.index("-cp") + 1]
            run("previous_factory_javac", ["javac", "--release", "17", "-cp", classpath,
                                           "-d", str(owned / "previous_classes"), str(previous)])
            regressions = all_command[:all_command.index("org.junit.runner.JUnitCore") + 1]
            regressions += ["com.skynet.codegenerator.cpp." + name for name in TESTS]
            regressions[regressions.index("-cp") + 1] = str(owned / "previous_classes") + os.pathsep + classpath
            failure = run("previous_factory_regressions", regressions, 1)
            match = re.search(r"Failures: (\d+)", failure.stdout)
            require(match is not None, "actual preceding-factory failures")
            record["regression_failure_count"] = int(match[1])
            record["compiled_sources"] = {name: sha(Path(name).read_bytes()) for name in command if name.endswith(".java")}
            record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
        record["inputs_after"] = execution_inputs()
        require(record["inputs_before"] == record["inputs_after"], "unchanged complete generation/test input frame")
        record["complete"] = True
    finally:
        record["owned_directory_removed"] = not Path(record.get("owned_directory", "/")).exists()
    return record


def validate_java(record):
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record.get("schema") == "teaql.approved_generation_decisions.java.v1", "complete scoped Java receipt")
    require(record["seed_sha256"] == SEED_SHA and sha(SEED.read_bytes()) == SEED_SHA, "exact command preparation")
    require(record["inputs_before"] == record["inputs_after"], "stable Java execution inputs")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0,
            "measured task-owned cleanup")
    require(type(record["java_test_count"]) is int and record["java_test_count"] == 289
            and type(record["regression_failure_count"]) is int and record["regression_failure_count"] == 5,
            "289 current Java tests and five preceding-factory regressions")
    steps = record["steps"]
    require([step["label"] for step in steps] == ["current_fresh_javac", "current_all_contracts",
            "previous_factory_javac", "previous_factory_regressions"]
            and all(type(step["exit_code"]) is int for step in steps)
            and [step["exit_code"] for step in steps] == [0, 0, 0, 1], "exact finished compiler/JUnit commands")
    require(re.search(r"OK \(289 tests\)", steps[1]["stdout"]) is not None
            and re.search(r"Failures: 5\b", steps[3]["stdout"]) is not None, "actual JUnit results")
    seed = read(SEED)
    expected_classes = next(step["command"] for step in seed["steps"] if step["label"] == "current_all_contracts")
    expected_classes = expected_classes[expected_classes.index("org.junit.runner.JUnitCore") + 1:]
    require(record["test_classes"] == expected_classes, "all retained Java test classes")
    require(set(record["source_images"]) == set(CHANGED), "one factory and three affected test files")
    for name, row in record["source_images"].items():
        for side in ("before", "after"):
            require(row[side + "_sha256"] == sha(row[side + "_source"].encode()), "full retained Java source image")
        require(sha((PRODUCER / name).read_bytes()) == row["after_sha256"], "current tested Java source: " + name)
    row = record["source_images"][JAVA]
    old = "      return new DomainGenerationService();\n"
    new = ('      return new DomainGenerationService(Map.of(CppCoreGenerator.SCOPE, source -> {\n'
           '        throw new IllegalStateException("C++ generation requires a configured locked compiler; set "\n'
           '            + PREFIX + "python, " + PREFIX + "worker, " + PREFIX + "capabilities and "\n'
           '            + PREFIX + "temporary_root in trusted startup configuration");\n'
           '      }));\n')
    require(row["after_source"].count(new) == 1 and row["after_source"].replace(new, old, 1) == row["before_source"],
            "only the missing-configuration branch changes in production")
    require(all(Path(name).is_file() and sha(Path(name).read_bytes()) == digest
                for name, digest in record["compiled_sources"].items()), "every freshly compiled Java input remains exact")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--qualify-java", action="store_true")
    parser.add_argument("--check-java", type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error("Preserve existing receipts")
    if args.qualify_java and (args.record is None or args.check_java):
        parser.error("Java execution requires a fresh --record and cannot use --check-java")
    contract = read(MANIFEST)
    validate_exceptions(contract)
    controls = exception_controls(contract)
    if args.qualify_java:
        result = qualify_java()
        write(args.record, result)
        validate_java(result)
    else:
        if args.check_java:
            validate_java(read(args.check_java))
        if args.record:
            write(args.record, dict(complete=True, whole_goal_complete=False,
                  scope="Approved behavior and immutable historical diagnostics; no new compiler or SQLite execution",
                  manifest_sha256=sha(MANIFEST.read_bytes()), reader_sha256=sha(Path(__file__).read_bytes()),
                  optimized=bool(__import__("sys").flags.optimize), controls=controls))
    print("Three decisions recorded; two independent approved scopes; 10 invalid approval controls rejected")


if __name__ == "__main__":
    main()
