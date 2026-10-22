#!/usr/bin/env python3
"""Replay retained metadata contracts against locked Rust and current Java sources.

Accepted inputs compare every metadata effect. Original refusals require a
checked refusal, without claiming identical cross-language error text/stages.
An optional parser overlay is a separately identified development proposal.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
REFERENCE = ROOT / ".reference/teaql-rs"
PROJECT = ROOT / "tests/parity/rust_container_metadata_reference_5_0_6"
PACKAGE = "com/skynet/codegenerator/cpp"
CORPUS = PRODUCER / "generator/src/test/resources/cpp_audit_metadata_5_0_6.json.gz"
TESTS = [
    "cpp/CppRustLiteralMetadataTest.java",
    "cpp/CppAuditMetadataSequenceTest.java",
    "cpp/CppContainerMetadataTest.java",
    "cpp/CppFieldLayoutMetadataTest.java",
    "cpp/CppJsonInputMetadataTest.java",
    "cpp/CppAuditMetadataCorpusTest.java",
    "CppGenerationTest.java",
    "CppIdentityGenerationTest.java",
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def identities(path):
    return {
        tuple(re.search(rf'^{field} = "([^"]+)"', block, re.M)[1]
              for field in ("name", "version", "source", "checksum"))
        for block in path.read_text().split("[[package]]")[1:]
        if '\nsource = "registry+' in block
    }


def same_contract(expected, actual):
    if expected.get("parsed"):
        return expected == actual
    return actual.get("parsed") is False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True,
                        help="Fresh compressed receipt; never replace a historical run")
    parser.add_argument("--candidate-parser", type=Path,
                        help="Compile an owned parser overlay instead of the installed source")
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != ".gz":
        parser.error("Use a fresh .gz evidence path")
    corpus = read(CORPUS)
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())
    generator = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())
    assert corpus["schema"] == 1 and len(corpus["cases"]) == 2665
    assert corpus["runtime_commit"] == runtime["commit"]
    assert corpus["generator_commit"] == generator["generator_baseline_commit"]
    original_receipt = ROOT / corpus["source_receipt"]
    assert digest(original_receipt) == corpus["source_receipt_sha256"]
    sources = sorted((PRODUCER / "generator/src/main/java" / PACKAGE).glob("*.java"))
    test_root = PRODUCER / "generator/src/test/java/com/skynet/codegenerator"
    tests = [test_root / name for name in TESTS]
    inputs = [*sources, *tests, CORPUS, Path(__file__).resolve(),
              ROOT / "tests/generator/ContainerMetadataProbe.java",
              PROJECT / "Cargo.toml", PROJECT / "Cargo.lock", PROJECT / "main.rs",
              REFERENCE / "Cargo.lock", REFERENCE / "teaql-macros/src/attr.rs"]
    if args.candidate_parser:
        inputs.append(args.candidate_parser.resolve())
    before = {str(path): digest(path) for path in inputs}
    record = {
        "scope": "Fresh complete original container parser and current Java sources, "
                 "finite retained metadata corpus plus permanent Java regressions; "
                 "not full Rust grammar, SDK/compiler or generator-goal acceptance",
        "complete": False,
        "full_goal_complete": False,
        "runtime_commit": runtime["commit"],
        "generator_commit": corpus["generator_commit"],
        "parser_overlay": str(args.candidate_parser) if args.candidate_parser else None,
        "input_hashes_before": before,
        "free_bytes_before": shutil.disk_usage(ROOT).free,
        "steps": [],
        "cases": [],
    }

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + "\n").encode(), mtime=0))

    def run(label, command, *, cwd=ROOT, env=None, input_text=None):
        print(label, "running", flush=True)
        result = subprocess.run(command, cwd=cwd, env=env, input=input_text,
                                text=True, capture_output=True, timeout=900)
        record["steps"].append({
            "label": label,
            "command": list(map(str, command)),
            "cwd": str(cwd),
            "exit_code": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        })
        save()
        print(label, "exit", result.returncode, flush=True)
        return result

    owned = None
    try:
        assert shutil.disk_usage(ROOT).free > 1024 ** 3
        check = run("upstream", ["python3", "scripts/verify_upstream.py"])
        check.check_returncode()
        expected = identities(REFERENCE / "Cargo.lock")
        actual = identities(PROJECT / "Cargo.lock")
        record["registry_identity_differences"] = sorted(actual - expected)
        assert not record["registry_identity_differences"]
        with tempfile.TemporaryDirectory(prefix="teaql_audit_metadata_source_506_") as name:
            owned = Path(name)
            record["owned_directory"] = name
            env = dict(os.environ, CARGO_TARGET_DIR=str(owned / "target"),
                       CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER="/usr/bin/cc")
            record["rust_environment"] = {key: value for key, value in env.items()
                                          if key.startswith("CARGO_")}
            built = run("original_build", ["cargo", "build", "--offline", "--locked",
                        "--manifest-path", str(PROJECT / "Cargo.toml"), "-j", "2"], env=env)
            built.check_returncode()
            encoded = "".join(json.dumps(case["input"], ensure_ascii=True) + "\n"
                              for case in corpus["cases"])
            observed = run("original", [str(owned / "target/debug/teaql_container_metadata_reference_506")],
                           input_text=encoded)
            observed.check_returncode()
            original = [json.loads(line) for line in observed.stdout.splitlines()]
            assert len(original) == len(corpus["cases"])
            record["original_corpus_differences"] = [
                {"name": case["name"], "input": case["input"],
                 "retained": case["expected"], "current": value}
                for case, value in zip(corpus["cases"], original)
                if case["expected"] != value
            ]
            assert not record["original_corpus_differences"], "Original source outcomes changed"

            classpath_file = owned / "classpath.txt"
            resolved = run("classpath", ["mvn", "-q", "-o", "-pl", "generator",
                           "dependency:build-classpath", f"-Dmdep.outputFile={classpath_file}"],
                           cwd=PRODUCER)
            resolved.check_returncode()
            cp = ":".join(str(PRODUCER / path) for path in
                          ("generator/target/classes", "model/target/classes"))
            cp += ":" + classpath_file.read_text().strip()
            classes = owned / "classes"
            classes.mkdir()
            copied = owned / "java"
            copied.mkdir()
            compiled_sources = []
            for source in sources:
                target = copied / source.name
                selected = (args.candidate_parser if args.candidate_parser
                            and source.name == "CppRustLiteralMetadata.java" else source)
                target.write_bytes(selected.read_bytes())
                compiled_sources.append(str(target))
            compiled = run("java_compile", ["javac", "-cp", cp, "-d", str(classes),
                           *compiled_sources, *map(str, tests),
                           str(ROOT / "tests/generator/ContainerMetadataProbe.java")])
            compiled.check_returncode()
            cp = ":".join(map(str, [classes, PRODUCER / "generator/src/test/resources",
                                   PRODUCER / "generator/src/main/resources"])) + ":" + cp
            java = run("java_metadata", ["java", "-cp", cp,
                       "com.skynet.codegenerator.cpp.ContainerMetadataProbe"], input_text=encoded)
            java.check_returncode()
            actual = [json.loads(line) for line in java.stdout.splitlines()]
            assert len(actual) == len(original)
            record["cases"] = [
                {"name": case["name"], "input": case["input"],
                 "original": expected, "java": value,
                 "match": same_contract(expected, value)}
                for case, expected, value in zip(corpus["cases"], original, actual)
            ]
            record["differences"] = [case for case in record["cases"] if not case["match"]]
            temporary = owned / "java_tmp"
            temporary.mkdir()
            names = ["com.skynet.codegenerator." + name[:-5].replace("/", ".")
                     for name in TESTS]
            junit = run("junit", ["java", "-Djava.io.tmpdir=" + str(temporary), "-cp", cp,
                        "org.junit.runner.JUnitCore", *names], cwd=PRODUCER / "generator")
            record["junit_passed"] = junit.returncode == 0
            summary = re.search(r"OK \((\d+) tests?\)", junit.stdout)
            record["junit_tests"] = int(summary[1]) if summary else None
            record["complete"] = not record["differences"] and record["junit_passed"]
            record["owned_bytes_removed"] = sum(path.stat().st_size
                                                for path in owned.rglob("*") if path.is_file())
    except Exception as error:
        record["controller_error"] = {"type": type(error).__name__, "message": str(error)}
        save()
        raise
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path): digest(path) for path in inputs}
        record["inputs_unchanged"] = before == record["input_hashes_after"]
        record["complete"] = record["complete"] and record["inputs_unchanged"]
        save()
    print("metadata cases:", len(record["cases"]), "differences:",
          len(record.get("differences", [])), "JUnit:", record.get("junit_tests"), flush=True)
    return 0 if record["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
