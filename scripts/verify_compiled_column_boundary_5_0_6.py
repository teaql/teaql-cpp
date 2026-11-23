#!/usr/bin/env python3
"""Qualify the approved source-column refusal for every compiled metadata format."""

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
PREVIOUS = "90ba3cddc6884cf73cca0055dd7025803cd29764"
SEED = ROOT / "docs/parity/option_keyword_java_5_0_6.json.gz"
SEED_SHA = "4f90469f4eb1698440bba3732dfe63cbcb4497b76e2b536db8e44e397da055bf"
CONFIGURATION = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerModelConfiguration.java"
TEST = "generator/src/test/java/com/skynet/codegenerator/cpp/CppCompilerModelConfigurationTest.java"
GUARD = ("        // Compiled facts do not authorize injecting attributes through a source column literal.\n"
         "        CppJsonInputMetadata.column(field);\n")


def require(condition, message):
    if not condition:
        raise ValueError("Compiled column boundary: " + message)


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

    require(not path.exists() and sha(SEED.read_bytes()) == SEED_SHA, "fresh record and immutable Java recipe")
    seed = read(SEED)
    record = dict(schema="teaql.compiled_column.execution.v1", complete=False, whole_goal_complete=False,
                  scope="All three trusted compiled-metadata readers reject the two source attribute injections; nine valid column cases preserve compiled authority. Synthetic metadata is transport evidence, not a successful original full-SDK compilation.",
                  steps=[], inputs_before=execution_inputs(), references_before=references(), source_images={})
    for name in (CONFIGURATION, TEST):
        before = subprocess.check_output(["git", "-C", str(PRODUCER), "show", PREVIOUS + ":" + name], text=True)
        after = (PRODUCER / name).read_text()
        record["source_images"][name] = dict(before_source=before, before_sha256=sha(before.encode()),
                                             after_source=after, after_sha256=sha(after.encode()))
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiled_column_java_506_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = temporary
            for name in ("cpp_classes", "java_tmp", "old_configuration_classes"):
                (owned / name).mkdir()

            def run(label, command, expected=0):
                result = run_owned(command, cwd=PRODUCER / "generator", text=True,
                                   capture_output=True, timeout=900)
                record["steps"].append(dict(label=label, command=command, exit_code=result.returncode,
                                             stdout=result.stdout, stderr=result.stderr))
                write(path, record)
                print(label, "exit", result.returncode, flush=True)
                require(result.returncode == expected, label + ": " + result.stdout[-4000:] + result.stderr[-2000:])
                return result

            old = seed["owned_directory"]
            compile_command = [arg.replace(old, temporary) for arg in seed["steps"][0]["command"]]
            run("current_fresh_javac", compile_command)
            command = [arg.replace(old, temporary) for arg in seed["steps"][1]["command"]]
            prefix = command[:command.index("org.junit.runner.JUnitCore") + 1]
            record["shared_test_classes"] = seed["shared_test_classes"]
            record["cpp_test_classes"] = seed["cpp_test_classes"]
            for label, classes in (("current_shared_junit", record["shared_test_classes"]),
                                   ("current_cpp_junit", record["cpp_test_classes"])):
                run(label, prefix + classes)
            classpath = prefix[prefix.index("-cp") + 1]
            source = owned / "CppCompilerModelConfiguration.java"
            source.write_text(record["source_images"][CONFIGURATION]["before_source"])
            run("old_configuration_javac", ["javac", "--release", "17", "-cp", classpath,
                "-d", str(owned / "old_configuration_classes"), str(source)])
            command = list(prefix)
            command[command.index("-cp") + 1] = str(owned / "old_configuration_classes") + os.pathsep + classpath
            run("old_configuration_regressions", command +
                ["com.skynet.codegenerator.cpp.CppCompilerModelConfigurationTest"], 1)
            record["compiled_sources"] = {name: sha(Path(name).read_bytes())
                                           for name in compile_command if name.endswith(".java")}
            record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(inputs_after=execution_inputs(), references_after=references(), complete=True)
        require(record["inputs_before"] == record["inputs_after"]
                and record["references_before"] == record["references_after"], "stable source/reference frame")
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record["owned_directory_removed"] = not Path(record.get("owned_directory", "/")).exists()
        write(path, record)


def validate(record):
    from text_alias_source_admission_5_0_6 import current_input_matches as text_matches

    require(sha(SEED.read_bytes()) == SEED_SHA, "immutable Java seed")
    require(record.get("schema") == "teaql.compiled_column.execution.v1" and record.get("complete") is True
            and record.get("whole_goal_complete") is False, "complete scoped execution")
    require(record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"], "unchanged execution inputs")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0,
            "measured owned cleanup")
    seed = read(SEED)
    require(record["shared_test_classes"] == seed["shared_test_classes"]
            and record["cpp_test_classes"] == seed["cpp_test_classes"], "complete JUnit class sets")
    steps = record["steps"]
    require([step["label"] for step in steps] == ["current_fresh_javac", "current_shared_junit",
        "current_cpp_junit", "old_configuration_javac", "old_configuration_regressions"]
        and all(type(step["exit_code"]) is int for step in steps)
        and [step["exit_code"] for step in steps] == [0, 0, 0, 0, 1], "completed actual compiler/test commands")
    require("OK (98 tests)" in steps[1]["stdout"] and "OK (297 tests)" in steps[2]["stdout"]
            and "Tests run: 12,  Failures: 1" in steps[4]["stdout"]
            and "compiledMetadataCannotBypassUnescapedColumnRefusal" in steps[4]["stdout"],
            "395 successful current tests; exact preceding implementation fails only the missing refusal")
    require(set(record["source_images"]) == {CONFIGURATION, TEST}, "one production guard and its regression tests")
    for name, row in record["source_images"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                == sha((PRODUCER / name).read_bytes()), "full exact source images")
    row = record["source_images"][CONFIGURATION]
    require(row["after_source"].count(GUARD) == 1
            and row["after_source"].replace(GUARD, "", 1) == row["before_source"],
            "only reuse the existing literal validator; compiled facts retain all other authority")
    require(all(Path(name).is_file() and (sha(Path(name).read_bytes()) == digest or text_matches(name, digest))
                for name, digest in record["compiled_sources"].items()), "all actual current Java compiler inputs")
    expected_driver = record["inputs_before"]["runtime:scripts/verify_compiled_column_boundary_5_0_6.py"]
    require(expected_driver == sha(Path(__file__).read_bytes()) or text_matches(Path(__file__), expected_driver),
            "actual collector source")


def controls(record):
    changes = [
        ("numeric_complete", lambda x: x.update(complete=1)),
        ("goal_promoted", lambda x: x.update(whole_goal_complete=True)),
        ("unfinished_compiler", lambda x: x["steps"][0].update(exit_code=1)),
        ("boolean_exit", lambda x: x["steps"][1].update(exit_code=False)),
        ("missing_tests", lambda x: x["cpp_test_classes"].pop()),
        ("missing_old_regression", lambda x: x["steps"][-1].update(exit_code=0)),
        ("wrong_failure", lambda x: x["steps"][-1].update(stdout="Tests run: 12,  Failures: 2")),
        ("changed_source", lambda x: x["source_images"][CONFIGURATION].update(after_source="pass")),
        ("changed_preimage", lambda x: x["source_images"][CONFIGURATION].update(before_sha256="0" * 64)),
        ("unstable_inputs", lambda x: x["inputs_after"].update({next(iter(x["inputs_after"])): "0" * 64})),
        ("lost_cleanup", lambda x: x.update(owned_directory_removed=False)),
        ("lost_compiler_input", lambda x: x["compiled_sources"].update({next(iter(x["compiled_sources"])): "0" * 64})),
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
            raise ValueError("Invalid compiled-column execution admitted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", required=True, type=Path)
    parser.add_argument("--controls-record", type=Path)
    args = parser.parse_args()
    require(not args.controls_record or not args.controls_record.exists(), "preserve previous control records")
    if args.execute:
        execute(args.record)
    record = read(args.record)
    validate(record)
    results = controls(record)
    if args.controls_record:
        write(args.controls_record, dict(complete=True, whole_goal_complete=False, results=results,
              execution_sha256=sha(args.record.read_bytes())))
    print("Compiled column guard qualified: 395 current Java tests; 12 corruptions rejected")


if __name__ == "__main__":
    main()
