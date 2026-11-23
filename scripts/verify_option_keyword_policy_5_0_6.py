#!/usr/bin/env python3
"""Qualify reserved Option names at the shared model and formal C++ boundaries."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
PREVIOUS = "90ba3cddc6884cf73cca0055dd7025803cd29764"
SEED = ROOT / "docs/parity/approved_generation_decisions_java_5_0_6.json.gz"
SEED_SHA = "dd610710e336b510e30b8dd2e5baa23d6079e7614f7cd5bcc859edcec9da23d8"
SHARED_SEED = ROOT / "docs/parity/generator_evaluation_full_5_0_6.json.gz"
SHARED_SHA = "b3623d2621aff942cc21a69b5ef90b07e57edc48709dea057f9d2a1d4045ed04"
RULE = "generator/src/main/java/com/skynet/codegenerator/evaluation/ReservedKeywordRule.java"
FACTORY = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerServiceConfiguration.java"
TESTS = ["generator/src/test/java/com/skynet/codegenerator/KsmlEvaluationControllerTest.java",
         "generator/src/test/java/com/skynet/codegenerator/cpp/CppCompilerHostConfigurationTest.java",
         "generator/src/test/java/com/skynet/codegenerator/cpp/CppOfflineCompilerConfigurationTest.java"]
MODEL = "generator/src/test/resources/models/cpp_option_reserved.xml"


def require(condition, message):
    if not condition:
        raise ValueError("Option keyword policy: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(Path(path).read_bytes()))


def write(path, value):
    Path(path).write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def execute(path):
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    from run_native_integration_epoch_hardened_5_0_6 import references
    from verify_primitive_const_governed_execution_5_0_6 import run_owned

    require(not path.exists(), "preserve prior executions")
    require(sha(SEED.read_bytes()) == SEED_SHA and sha(SHARED_SEED.read_bytes()) == SHARED_SHA,
            "immutable Java preparation and full shared-evaluation recipes")
    seed, shared_seed = read(SEED), read(SHARED_SEED)
    record = dict(schema="teaql.option_keyword.execution.v1", complete=False, whole_goal_complete=False,
                  scope="Shared model keyword validation and actual configured C++ service/HTTP/offline boundaries; stub worker journals prove invalid models do not invoke the compiler. No new Rust or C++ compiler execution.",
                  steps=[], inputs_before=execution_inputs(), references_before=references(), source_images={})
    for name in [RULE, FACTORY, *TESTS]:
        before = subprocess.check_output(["git", "-C", str(PRODUCER), "show", PREVIOUS + ":" + name], text=True)
        after = (PRODUCER / name).read_text()
        record["source_images"][name] = dict(before_source=before, before_sha256=sha(before.encode()),
                                             after_source=after, after_sha256=sha(after.encode()))
    require(record["source_images"][RULE]["before_source"] == (ROOT / ".reference/teaql_code_gen" / RULE).read_text(),
            "unchanged locked reference keyword rule")
    model = (PRODUCER / MODEL).read_text()
    old_case, = read(ROOT / "docs/parity/ap02_plain_option_model_diagnostic_5_0_6.json.gz")["cases"]
    require(model == old_case["model"], "exact previously failing complete Option model")
    record["recorded_model"] = dict(path=MODEL, source=model, sha256=sha(model.encode()))
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_option_keyword_506_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = str(owned)
            for name in ("cpp_classes", "java_tmp", "old_rule_classes", "old_factory_classes"):
                (owned / name).mkdir()

            def run(label, command, expected=0):
                result = run_owned(command, cwd=PRODUCER / "generator", text=True,
                                   capture_output=True, timeout=900)
                record["steps"].append(dict(label=label, command=command, exit_code=result.returncode,
                                             stdout=result.stdout, stderr=result.stderr))
                write(path, record)
                print(label, "exit", result.returncode, flush=True)
                require(result.returncode == expected, label + ": " + result.stdout[-4000:] + result.stderr[-4000:])
                return result

            old = seed["owned_directory"]
            compile_command = [part.replace(old, temporary) for part in seed["steps"][0]["command"]]
            for name in TESTS:
                if str(PRODUCER / name) not in compile_command:
                    compile_command.append(str(PRODUCER / name))
            run("current_fresh_javac", compile_command)
            cpp_command = [part.replace(old, temporary) for part in seed["steps"][1]["command"]]
            prefix = cpp_command[:cpp_command.index("org.junit.runner.JUnitCore") + 1]
            record["cpp_test_classes"] = seed["test_classes"]
            record["shared_test_classes"] = shared_seed["shared_test_classes"]
            shared = run("current_shared_junit", prefix + record["shared_test_classes"])
            cpp = run("current_cpp_junit", prefix + record["cpp_test_classes"])
            for name, result in (("shared_test_count", shared), ("cpp_test_count", cpp)):
                count = re.search(r"OK \((\d+) tests\)", result.stdout)
                require(count is not None, "actual successful JUnit result")
                record[name] = int(count[1])
            selected = ["com.skynet.codegenerator.KsmlEvaluationControllerTest",
                        "com.skynet.codegenerator.cpp.CppCompilerHostConfigurationTest",
                        "com.skynet.codegenerator.cpp.CppOfflineCompilerConfigurationTest"]
            classpath = prefix[prefix.index("-cp") + 1]
            for label, name, folder in (("old_rule", RULE, "old_rule_classes"),
                                       ("old_factory", FACTORY, "old_factory_classes")):
                source = owned / Path(name).name
                source.write_text(record["source_images"][name]["before_source"])
                run(label + "_javac", ["javac", "--release", "17", "-cp", classpath,
                                       "-d", str(owned / folder), str(source)])
                command = list(prefix)
                command[command.index("-cp") + 1] = str(owned / folder) + os.pathsep + classpath
                regression = run(label + "_regressions", command + selected, 1)
                count = re.search(r"Failures: (\d+)", regression.stdout)
                require(count is not None, "actual previous-source regression failures")
                record[label + "_failure_count"] = int(count[1])
            record["compiled_sources"] = {name: sha(Path(name).read_bytes()) for name in compile_command if name.endswith(".java")}
            record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(inputs_after=execution_inputs(), references_after=references(), complete=True)
        require(record["inputs_before"] == record["inputs_after"]
                and record["references_before"] == record["references_after"], "stable full source/reference frame")
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record["owned_directory_removed"] = not Path(record.get("owned_directory", "/")).exists()
        write(path, record)


def validate(record):
    from compiled_column_source_admission_5_0_6 import current_input_matches as column_matches

    require(record.get("schema") == "teaql.option_keyword.execution.v1" and record.get("complete") is True
            and record.get("whole_goal_complete") is False, "complete scoped execution")
    require(record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"], "stable complete source/reference frame")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0, "measured owned cleanup")
    require(type(record["shared_test_count"]) is int and record["shared_test_count"] == 98
            and type(record["cpp_test_count"]) is int and record["cpp_test_count"] == 295,
            "all shared evaluation and C++ generator tests")
    require(type(record["old_rule_failure_count"]) is int and record["old_rule_failure_count"] == 6
            and type(record["old_factory_failure_count"]) is int and record["old_factory_failure_count"] == 3,
            "exact previous-list and missing-boundary regressions")
    require(record["shared_test_classes"] == read(SHARED_SEED)["shared_test_classes"]
            and record["cpp_test_classes"] == read(SEED)["test_classes"], "complete retained JUnit class sets")
    steps = record["steps"]
    require([row["label"] for row in steps] == ["current_fresh_javac", "current_shared_junit", "current_cpp_junit",
            "old_rule_javac", "old_rule_regressions", "old_factory_javac", "old_factory_regressions"]
            and all(type(row["exit_code"]) is int for row in steps)
            and [row["exit_code"] for row in steps] == [0, 0, 0, 0, 1, 0, 1], "finished compiler and actual JUnit commands")
    require("OK (98 tests)" in steps[1]["stdout"] and "OK (295 tests)" in steps[2]["stdout"]
            and "Failures: 6" in steps[4]["stdout"] and "Failures: 3" in steps[6]["stdout"], "actual retained outcomes")
    require(set(record["source_images"]) == {RULE, FACTORY, *TESTS}, "one central keyword rule, one factory and three test files")
    for name, row in record["source_images"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                == sha((PRODUCER / name).read_bytes()), "exact tested source image")
    require(all(Path(name).is_file() and (sha(Path(name).read_bytes()) == digest
                or column_matches(Path(name), digest))
                for name, digest in record["compiled_sources"].items()), "every current compiled Java source")
    require(record["recorded_model"] == dict(path=MODEL, source=(PRODUCER / MODEL).read_text(),
            sha256=sha((PRODUCER / MODEL).read_bytes())), "same complete recorded model")


def controls(record):
    changes = [
        ("numeric_complete", lambda x: x.update(complete=1)),
        ("goal_promoted", lambda x: x.update(whole_goal_complete=True)),
        ("missing_shared_test", lambda x: x["shared_test_classes"].pop()),
        ("boolean_test_count", lambda x: x.update(cpp_test_count=True)),
        ("missing_keyword_regression", lambda x: x.update(old_rule_failure_count=5)),
        ("missing_boundary_regression", lambda x: x.update(old_factory_failure_count=2)),
        ("failed_compiler", lambda x: x["steps"][0].update(exit_code=1)),
        ("unfinished_execution", lambda x: x["steps"].pop()),
        ("changed_source", lambda x: x["source_images"][RULE].update(after_source="pass")),
        ("changed_before_epoch", lambda x: x["inputs_after"].update({next(iter(x["inputs_after"])): "0" * 64})),
        ("lost_cleanup", lambda x: x.update(owned_directory_removed=False)),
        ("changed_recorded_model", lambda x: x["recorded_model"].update(source="<root/>")),
    ]
    results = []
    for name, change in changes:
        altered = copy.deepcopy(record)
        change(altered)
        try:
            validate(altered)
        except (ValueError, KeyError, TypeError, IndexError) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise ValueError("Invalid keyword acceptance admitted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--controls-record", type=Path)
    args = parser.parse_args()
    if args.controls_record and args.controls_record.exists():
        parser.error("Preserve prior control evidence")
    if args.execute:
        execute(args.record)
    record = read(args.record)
    validate(record)
    results = controls(record)
    if args.controls_record:
        write(args.controls_record, dict(complete=True, whole_goal_complete=False,
              execution_sha256=sha(args.record.read_bytes()), optimized=bool(sys.flags.optimize), results=results))
    print("Option names rejected by shared evaluation and formal C++ generation; 393 Java contracts pass; 12 corruptions reject")


if __name__ == "__main__":
    main()
