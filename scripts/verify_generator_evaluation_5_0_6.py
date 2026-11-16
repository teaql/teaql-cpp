#!/usr/bin/env python3
"""Compile and compare the locked shared evaluator and the C++ producer repair."""

import argparse
import base64
import copy
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
ORIGINAL = ROOT / ".reference/teaql_code_gen"
SEED = ROOT / "docs/parity/compiler_model_configuration_full_5_0_6.json.gz"
SEED_SHA = "3d7a47edf73681f2c6e9700c5df0067fff8750eddcd94186053e3ca22350cef7"
PROBE = PRODUCER / "tools/cpp_evaluation/CppEvaluationProbe.java"
EVALUATION = Path("generator/src/main/java/com/skynet/codegenerator/evaluation")
TESTS = Path("generator/src/test/java/com/skynet/codegenerator/evaluation")
SEMANTIC = Path("generator/src/main/java/com/skynet/codegenerator/semantic")
SHARED_TESTS = Path("generator/src/test/java/com/skynet/codegenerator/semantic")
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same


def require(value, message):
    if not value:
        raise RuntimeError("Generator evaluation: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def imported_sources():
    paths = [path.relative_to(ORIGINAL) for folder in (EVALUATION, TESTS)
             for path in sorted((ORIGINAL / folder).glob("*.java"))]
    paths.extend(SEMANTIC / (name + ".java") for name in (
        "SemanticIdentityService", "SemanticIdentityBinding", "SemanticIdentityDocument"))
    paths.extend((SHARED_TESTS / "SemanticIdentityServiceTest.java",
                  Path("model/src/main/java/com/skynet/codegenerator/naming/FieldDescriptorNaming.java"),
                  Path("generator/src/main/java/com/skynet/codegenerator/ValidatedGeneration.java"),
                  Path("generator/src/main/java/com/skynet/codegenerator/LocalGenerator.java"),
                  Path("generator/src/test/java/com/skynet/codegenerator/GenerationEvaluationGateTest.java"),
                  Path("generator/src/test/java/com/skynet/codegenerator/KsmlEvaluationControllerTest.java"),
                  Path("generator/src/main/java/com/skynet/codegenerator/input/ModelInputLoader.java")))
    return paths


def source_package(files, entrance="main.xml"):
    return dict(schema="teaql.model_source_package.v1", entrance_file=entrance,
                files={name: base64.b64encode(value.encode()).decode() for name, value in files.items()})


def cases():
    baseline = (ROOT / "tests/generator/inheritance_graph.xml").read_text()
    basic = '<root name="evaluation_fixture" org="teaql" data_service="sqlite"><platform name="Platform"/>'
    suffix = '</root>'
    return [
        dict(name="retained_inheritance", package=source_package({"main.xml": baseline})),
        dict(name="registered_scalars", package=source_package({"main.xml": basic
             + '<reading platform="platform()" count="integer()" active="boolean()" '
             'value="number()" label="string()"/>' + suffix})),
        dict(name="unknown_reference", package=source_package({"main.xml": basic
             + '<reading platform="platform()" missing="does_not_exist()"/>' + suffix})),
        dict(name="business_id", package=source_package({"main.xml": basic
             + '<commerce_order platform="platform()" order_number="businessId(prefix=ORD)"/>' + suffix})),
        dict(name="business_id_legacy", package=source_package({"main.xml": basic
             + '<commerce_order platform="platform()" order_number="business_id(profile=daily-sequence prefix=ORD preserve_digits=8)"/>' + suffix})),
        dict(name="invalid_business_id", package=source_package({"main.xml": basic
             + '<commerce_order platform="platform()" order_number="businessId(profile=uuid preserve_digits=20)"/>' + suffix})),
        dict(name="invalid_sidecar", package=source_package({"main.xml": basic + suffix,
             ".teaql/semantic-ids.json": '{"schemaVersion":1,"modelId":"invalid","bindings":[]}'})),
        dict(name="malformed_sidecar", package=source_package({"main.xml": basic + suffix,
             ".teaql/semantic-ids.json": '{'})),
        dict(name="multifile_reference", package=source_package({"main.xml": basic
             + '<_include file="parts/reading.xml"/>' + suffix,
             "parts/reading.xml": '<root>\n<reading platform="platform()" count="integer()"/>\n</root>'})),
        dict(name="missing_include", package=source_package({"main.xml": basic
             + '<_include file="missing.xml"/>' + suffix})),
        dict(name="recursive_include", package=source_package({"main.xml": basic
             + '<_include file="part.xml"/>' + suffix,
             "part.xml": '<root><_include file="main.xml"/></root>'})),
        dict(name="malformed_xml", package=source_package({"main.xml": '<root>\n<reading></root>'})),
        dict(name="privacy", package=source_package({"main.xml": basic
             + '<customer platform="platform()" name="Ada" email="string()" national_id="string()"/>' + suffix})),
        dict(name="invalid_mask", package=source_package({"main.xml": basic
             + '<customer platform="platform()" name="Ada" _audit_mask_fields="missing"/>' + suffix})),
    ]


def prepare(directory, record, run):
    require(sha(SEED.read_bytes()) == SEED_SHA, "immutable preparation recipe")
    seed = read(SEED)
    old = seed["owned_directory"]
    shared = imported_sources()
    for relative in shared:
        require((ORIGINAL / relative).read_bytes() == (PRODUCER / relative).read_bytes(),
                "exact locked shared responsibility: " + str(relative))
    for label in ("original", "cpp"):
        (directory / (label + "_classes")).mkdir()
        seed_label = "original_bridge_javac" if label == "original" else "cpp_javac"
        command = [part.replace(old, str(directory)) for part in
                   next(step["command"] for step in seed["steps"] if step["label"] == seed_label)]
        workspace = ORIGINAL if label == "original" else PRODUCER
        extra = [workspace / relative for relative in shared]
        extra.extend((PROBE, PRODUCER / "tools/cpp_model_source/CppModelSource.java",
                      workspace / "generator/src/test/java/com/skynet/codegenerator/KsmlEvaluationControllerTest.java"))
        if label == "cpp":
            extra.append(PRODUCER / "generator/src/test/java/com/skynet/codegenerator/CppValidatedGenerationTest.java")
        for source in extra:
            if str(source) not in command:
                command.append(str(source))
        run(label + "_javac", command)
    classpaths = {label: seed["capabilities"][key]["command"][2].replace(old, str(directory))
                  for label, key in (("original", "reference_generator"), ("cpp", "cpp_generator"))}
    record["compiled_sources"] = {str(Path(part)): sha(Path(part).read_bytes())
                                  for step in record["steps"] for part in step["command"]
                                  if part.endswith(".java")}
    return classpaths


def execute(path):
    require(not path.exists(), "preserve execution evidence")
    record = dict(complete=False, whole_goal_complete=False, steps=[],
                  driver_source=Path(__file__).read_text(), probe_source=PROBE.read_text(),
                  input_hashes_before=capture_inputs(), references_before=references(), cases=cases())

    def run(label, command, *, cwd=ROOT):
        command = list(map(str, command))
        result = run_owned(command, cwd=cwd, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, cwd=str(cwd),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(result.returncode == 0, "actual Java command failed: " + label + "; see " + str(path))
        return result

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_generator_evaluation_") as temporary:
            directory = Path(temporary)
            record["owned_directory"] = temporary
            try:
                classpaths = prepare(directory, record, run)
                shared_tests = ["com.skynet.codegenerator.evaluation." + p.stem
                                for p in sorted((ORIGINAL / TESTS).glob("*Test.java"))]
                shared_tests.append("com.skynet.codegenerator.semantic.SemanticIdentityServiceTest")
                shared_tests.extend(("com.skynet.codegenerator.GenerationEvaluationGateTest",
                                     "com.skynet.codegenerator.KsmlEvaluationControllerTest"))
                record["shared_test_classes"] = shared_tests
                for label, workspace in (("original", ORIGINAL), ("cpp", PRODUCER)):
                    test_directory = directory / (label + "_test_outputs")
                    test_directory.mkdir()
                    result = run(label + "_shared_junit", ["java", "-Djava.io.tmpdir=" + str(test_directory), "-cp", classpaths[label],
                                 "org.junit.runner.JUnitCore", *shared_tests], cwd=workspace / "generator")
                    count = re.search(r"OK \((\d+) tests\)", result.stdout)
                    require(count is not None, "actual full shared Java unit outcome")
                    record[label + "_shared_test_count"] = int(count[1])
                seed = read(SEED)
                command = list(next(step["command"] for step in seed["steps"] if step["label"] == "cpp_junit"))
                command[command.index("-cp") + 1] = classpaths["cpp"]
                command.append("com.skynet.codegenerator.CppValidatedGenerationTest")
                result = run("cpp_generator_junit", command, cwd=PRODUCER / "generator")
                count = re.search(r"OK \((\d+) tests\)", result.stdout)
                require(count is not None, "actual current generator Java contracts")
                record["cpp_generator_test_count"] = int(count[1])
                inputs = []
                for case in record["cases"]:
                    package = directory / (case["name"] + ".json")
                    package.write_text(json.dumps(case["package"], sort_keys=True))
                    inputs.append(package)
                for label in ("original", "cpp"):
                    result = run(label + "_observations", ["java", "-cp", classpaths[label],
                                 "CppEvaluationProbe", *inputs])
                    values = [json.loads(line[len("TEAQL_EVALUATION_JSON="):]) for line in result.stdout.splitlines()
                              if line.startswith("TEAQL_EVALUATION_JSON=")]
                    require(len(values) == len(record["cases"]), "complete actual evaluation output")
                    for case, value in zip(record["cases"], values, strict=True):
                        case[label] = value
                record["differences"] = [case["name"] for case in record["cases"] if not same(case["original"], case["cpp"])]
                require(record["differences"] == [], "actual independent evaluation differs")
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in directory.rglob("*") if p.is_file())
        record.update(input_hashes_after=capture_inputs(), references_after=references(),
                      owned_directory_removed=not directory.exists(), complete=True)
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    require(sha(SEED.read_bytes()) == SEED_SHA, "exact retained preparation recipe")
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["owned_directory_removed"] is True, "complete scoped evaluation")
    require(record["driver_source"] == Path(__file__).read_text() and record["probe_source"] == PROBE.read_text(),
            "actual current observation sources")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"], "stable source and reference inputs")
    require(record["differences"] == [] and len(record["cases"]) == len(cases()), "complete independent comparison")
    for actual, expected in zip(record["cases"], cases(), strict=True):
        require(actual["name"] == expected["name"] and actual["package"] == expected["package"]
                and same(actual["original"], actual["cpp"]), "actual unchanged source and complete evaluation output")
    require(all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["steps"]),
            "all actual Java commands succeeded")
    require([step["label"] for step in record["steps"]] == ["original_javac", "cpp_javac", "original_shared_junit",
            "cpp_shared_junit", "cpp_generator_junit", "original_observations", "cpp_observations"], "exact Java command inventory")
    require(record["original_shared_test_count"] == record["cpp_shared_test_count"] == 95
            and record["cpp_generator_test_count"] == 259, "full locked shared and current generator tests")
    expected_sources = {part for step in record["steps"][:2]
                        for part in step["command"] if part.endswith(".java")}
    require(set(record["compiled_sources"]) == expected_sources
            and all(Path(name).is_file() and sha(Path(name).read_bytes()) == value
                for name, value in record["compiled_sources"].items()), "current compiled source identities")
    steps = {step["label"]: step for step in record["steps"]}
    for label, field in (("original_shared_junit", "original_shared_test_count"),
                         ("cpp_shared_junit", "cpp_shared_test_count"),
                         ("cpp_generator_junit", "cpp_generator_test_count")):
        count = re.search(r"OK \((\d+) tests\)", steps[label]["stdout"])
        require(type(record[field]) is int and count is not None and int(count[1]) == record[field],
                "actual JUnit stdout: " + label)
    for language in ("original", "cpp"):
        values = [json.loads(line[len("TEAQL_EVALUATION_JSON="):])
                  for line in steps[language + "_observations"]["stdout"].splitlines()
                  if line.startswith("TEAQL_EVALUATION_JSON=")]
        require(same(values, [case[language] for case in record["cases"]]), "actual complete observation stdout")
    by_name = {case["name"]: case["cpp"] for case in record["cases"]}
    for name in ("retained_inheritance", "registered_scalars", "business_id", "business_id_legacy", "multifile_reference"):
        require(by_name[name]["response"]["errors"] == [], "valid source is accepted: " + name)
    for name, rule in (("unknown_reference", "KSML-REFERENCE-003"), ("invalid_business_id", "KSML-BUSINESS-ID-002"),
                       ("invalid_sidecar", "KSML-SEMANTIC-ID-002"), ("malformed_sidecar", "KSML-SEMANTIC-ID-001"),
                       ("missing_include", "KSML-UPLOAD-006"), ("recursive_include", "KSML-UPLOAD-007"),
                       ("malformed_xml", "KSML-XML-001"), ("invalid_mask", "KSML-PRIVACY-002-MASK-FIELD")):
        require(any(item["ruleId"] == rule for item in by_name[name]["response"]["errors"]),
                "source error remains enforced: " + name)


def controls(record):
    validate(record)
    changes = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_case", lambda r: r["cases"].pop()),
        ("changed_source", lambda r: r["cases"][0]["package"].update(entrance_file="other.xml")),
        ("wrong_findings", lambda r: r["cases"][0]["cpp"]["response"]["errors"].append(dict(ruleId="wrong"))),
        ("missing_compile", lambda r: r["steps"].pop(0)),
        ("boolean_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("missing_tests", lambda r: r.update(cpp_generator_test_count=0)),
        ("changed_probe", lambda r: r.update(probe_source="pass")),
        ("coerced_test_count", lambda r: r.update(original_shared_test_count=True, cpp_shared_test_count=True)),
        ("wrong_test_count", lambda r: r.update(cpp_shared_test_count=r["cpp_shared_test_count"] + 1)),
        ("wrong_stdout", lambda r: r["steps"][-1].update(stdout="TEAQL_EVALUATION_JSON={}\n")),
        ("changed_java", lambda r: r["compiled_sources"].update({next(iter(r["compiled_sources"])): "0" * 64})),
        ("missing_java", lambda r: r["compiled_sources"].pop(next(iter(r["compiled_sources"])))),
    )
    rejected = []
    for name, mutate in changes:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (RuntimeError, KeyError) as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted evaluation corruption: " + name)
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path)
    parser.add_argument("--check", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.execute.resolve())
    elif args.check and args.output:
        require(not args.output.exists(), "preserve corruption evidence")
        rejected = controls(read(args.check))
        write(args.output, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                                execution_sha256=sha(args.check.read_bytes()), controls=rejected))
        print("Verified evaluation and", len(rejected), "corruptions; optimized=", sys.flags.optimize)
    else:
        parser.error("Choose --execute or --check with --output")


if __name__ == "__main__":
    main()
