#!/usr/bin/env python3
"""Qualify installed scalar constant projection generation and preserved contracts."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import traceback

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_audit_metadata_source_5_0_6 import TESTS
from verify_nominal_type_installation_5_0_6 import digest, files, read, require

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "candidate": [
        "ap02_scalar_const_value_candidate_v3_5_0_6.json.gz",
        "9686dcf2f0eb7a42f3e27e8b99bf7b8827e4ecea8f48d15465d7772d31a0ca10"
    ],
    "preceding": [
        "ap02_array_const_expression_installed_5_0_6.json.gz",
        "4b4ba9fcddd2e349b746834975cd0da5f4a3122a98ea4c11cacaf39423abf1fe"
    ],
    "partial": [
        "ap02_scalar_const_value_candidate_5_0_6.json.gz",
        "8242c17120fb0ff573cdf1db05d812e9015c7ea4cdf142d89758c358a776875f"
    ],
    "literal": [
        "ap02_scalar_const_literal_generation_diagnostic_5_0_6.json.gz",
        "a607fcf8efde5e0c672f817fd6cf3f060539594c1a8aab5ec278bd95321203dd"
    ],
    "matrix": [
        "ap02_scalar_cast_const_matrix_5_0_6.json.gz",
        "34991adc391729df5ee76ede2eff25167038efb05c515ecef36361d7f75748d3"
    ]
}
UNIT_NAMES = (
    "CppNominalTypeProjectionTest", "CppQualifiedEntityTypeProjectionTest",
    "CppContainerTypeProjectionTest", "CppResultTypeProjectionTest",
    "CppStandardValueTypeProjectionTest", "CppSmartListTypeProjectionTest",
    "CppArrayConstExpressionProjectionTest", "CppScalarConstValueProjectionTest",
)


def bound(key):
    name, expected = RECORDS[key]
    path = EVIDENCE / name
    require(digest(path) == expected, "Immutable compiler witness: " + name)
    return read(path)


def selected_cases(candidate, preceding):
    matrix, partial, literal = bound("matrix"), bound("partial"), bound("literal")
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["junit_tests"] == 203
            and candidate["differences"] == [] and len(candidate["cases"]) == 65
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["owned_directory_removed"] is True
            and candidate["cast_matrix_sha256"] == RECORDS["matrix"][1],
            "Complete scalar compiler and permanent regression qualification")
    require(matrix["complete"] is True and len(matrix["cases"]) == 294
            and matrix["owned_directory_removed"] is True, "Complete const cast source matrix")
    require(preceding["complete"] is True and preceding["installed"] is True
            and preceding["junit_tests"] == 194 and len(preceding["cases"]) == 193,
            "All preceding production SDK/compiler contracts")
    require(candidate["prior_partial_sha256"] == RECORDS["partial"][1]
            and candidate["literal_generation_sha256"] == RECORDS["literal"][1]
            and candidate["reused_cpp_compiler_outcomes"] == 36
            and candidate["actual_cpp_contract_compiler_processes"] == 29
            and candidate["fresh_cpp_contract_compiler_processes"] == 29,
            "Exact 29 fresh and 36 explicitly reused compiler outcomes")
    require(len(partial["cases"]) == 36 and partial["inputs_unchanged"] is True
            and partial["references_unchanged"] is True
            and partial["owned_directory_removed"] is True,
            "Completed outcomes from the preserved partial controller")
    old_cases = {row["name"]: row for row in partial["cases"]}
    reused = [row for row in candidate["cases"]
              if row.get("compiler_outcomes_explicitly_reused") is True]
    require(len(reused) == 36 and {row["name"] for row in reused} == set(old_cases),
            "Exact reused compiler selection")
    for row in reused:
        old = old_cases[row["name"]]
        require(row["source_receipt_sha256"] == RECORDS["partial"][1]
                and all(row[key] == old[key] for key in (
                    "model", "type", "cpp_files", "original_files", "cpp_exit_code",
                    "original_exit_code", "consumer_source", "consumer_manifest", "consumer_lock")),
                "Byte-identical SDKs and actual completed consumer outcomes")
    require(literal["complete"] is True and len(literal["cases"]) == 9
            and all(row["original_exit_code"] == 0 for row in literal["cases"]),
            "Actual generated-context literal witnesses")
    require(candidate["candidate_resource"]["source_reference_sha256"] == RECORDS["matrix"][1],
            "Cast fixture refers to the immutable Rust observations")
    result = []
    for prefix, owner, selections, file_key, code_key in (
        ("preserved_", "preceding", preceding["cases"], "files", "reused_cpp_exit_code"),
        ("scalar_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in selections:
            result.append(dict(name=prefix + row["name"], source_case=row["name"],
                               source_record=owner, model=row["model"], type=row["type"],
                               files=row[file_key], original_exit_code=row["original_exit_code"],
                               reused_cpp_exit_code=row[code_key]))
    require(len(result) == len({row["name"] for row in result}) == 258,
            "All 193 preserved and 65 scalar compiler contracts")
    require(all(type(row["reused_cpp_exit_code"]) is int
                and (row["reused_cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                for row in result), "Original/native compiler outcomes")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    require(not args.record.exists(), "Preserve earlier qualification receipts")
    require(sys.flags.optimize == 0,
            "Run generation with ordinary Python; acceptance predicates have separate dual-mode controls")
    require(shutil.disk_usage(ROOT).free > 1024 ** 3, "Owned production verification space")
    candidate, preceding = bound("candidate"), bound("preceding")
    cases = selected_cases(candidate, preceding)
    for name, row in candidate["candidate_overlays"].items():
        require(digest(Path(name)) == row["after_sha256"], "Exact reviewed installed implementation")
    unit_root = PRODUCER / "generator/src/test/java/com/skynet/codegenerator"
    unit = unit_root / "cpp/CppScalarConstValueProjectionTest.java"
    require(digest(unit) == candidate["candidate_unit_sha256"], "Nine permanent scalar regression methods")
    cast_resource = PRODUCER / candidate["candidate_resource"]["path"]
    require(digest(cast_resource) == candidate["candidate_resource"]["sha256"], "Immutable cast fixture")
    for name, row in candidate["candidate_test_overlays"].items():
        require(digest(Path(name)) == row["after_sha256"],
                "Existing test expectation follows actual generated source")
    maven = shutil.which("mvn")
    require(maven is not None, "Put the installed Maven executable on PATH")
    sources = sorted(path for package in ("model", "generator")
                     for path in (PRODUCER / package / "src/main/java").rglob("*.java"))
    tests = [unit_root / name for name in TESTS]
    tests.extend(unit_root / ("cpp/" + name + ".java") for name in UNIT_NAMES)
    headers = sorted(path for package in ROOT.glob("teaql_*")
                     for suffix in ("*.hpp", "*.h") for path in (package / "include").rglob(suffix))
    inputs = [Path(__file__), ROOT / "scripts/verify_audit_metadata_source_5_0_6.py",
              ROOT / "scripts/verify_nominal_type_installation_5_0_6.py",
              ROOT / "upstream.lock.json", ROOT / "toolchains.lock.json",
              PRODUCER / "cpp_generator.lock.json",
              cast_resource, *[EVIDENCE / value[0] for value in RECORDS.values()], *sources, *tests, *headers]
    before = {str(path.resolve()): digest(path) for path in inputs}
    global_before, refs_before = capture_inputs(), references()
    jars = candidate["java_dependencies"]
    require(jars == preceding["dependency_jars"]
            and all(digest(Path(name)) == value for name, value in jars.items()),
            "Identical qualified Java dependency set")
    compiler = candidate["native_compiler"]
    require(compiler == preceding["native_compiler"]
            and digest(Path(compiler["path"])) == compiler["sha256"], "Retained actual native compiler")
    record = dict(complete=False, installed=True, whole_goal_complete=False,
                  scope="Fresh production Java and 258 complete SDKs; all compiler outcomes explicitly reused through exact whole-SDK equality; zero new C++ compiler processes",
                  candidate_sha256=RECORDS["candidate"][1],
                  preceding_sha256=RECORDS["preceding"][1], input_hashes_before=before,
                  global_inputs_before=global_before, global_references_before=refs_before,
                  dependency_jars=jars, native_compiler=compiler, new_cpp_compiler_processes=0,
                  reused_cpp_compiler_outcomes=258, steps=[], cases=[],
                  producer_commit=subprocess.check_output(
                      ["git", "-C", str(PRODUCER), "rev-parse", "HEAD"], text=True).strip())

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))

    def run(label, command, cwd=ROOT):
        command = list(map(str, command))
        print(label + " running", flush=True)
        start = time.monotonic()
        child = subprocess.Popen(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, start_new_session=True)
        try:
            out, err = child.communicate(timeout=1200)
        except BaseException:
            try:
                os.killpg(child.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                child.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.communicate()
            raise
        record["steps"].append(dict(label=label, command=command, cwd=str(cwd),
                                    exit_code=child.returncode, stdout=out, stderr=err,
                                    duration_seconds=time.monotonic() - start))
        save()
        require(child.returncode == 0, label + " failed: " + err[-2000:])
        return out

    owned = None
    error = None
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_scalar_const_installed_506_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                run("producer_package", [maven, "-q", "-o", "-pl", "generator", "-am",
                                         "-DskipTests", "package"], PRODUCER)
                classes = owned / "classes"
                classes.mkdir()
                cp_base = ":".join(jars)
                run("production_java_compile", ["javac", "--release", "17", "-cp", cp_base,
                                                "-d", classes, *sources, *tests])
                cp = ":".join(map(str, [classes, PRODUCER / "generator/src/main/resources",
                                       PRODUCER / "model/src/main/resources",
                                       PRODUCER / "generator/src/test/resources"])) + ":" + cp_base
                names = ["com.skynet.codegenerator." + name[:-5].replace("/", ".") for name in TESTS]
                names.extend("com.skynet.codegenerator.cpp." + name for name in UNIT_NAMES)
                output = run("production_junit", ["java", "-Djava.io.tmpdir=" + directory,
                             "-cp", cp, "org.junit.runner.JUnitCore", *names], PRODUCER / "generator")
                require(re.search(r"OK \(203 tests\)", output) is not None,
                        "All 203 permanent production Java tests")
                record["junit_tests"] = 203
                for case in cases:
                    folder = owned / case["name"]
                    folder.mkdir()
                    xml, sdk = folder / "model.xml", folder / "cpp"
                    xml.write_text(case["model"])
                    run(case["name"] + "_generate", ["java", "-cp", cp,
                        "com.skynet.codegenerator.cpp.CppCoreGenerator", xml, sdk])
                    generated = files(sdk)
                    require(generated == case["files"], "Exact complete production SDK: " + case["name"])
                    record["cases"].append(dict(
                        **case, compiler_outcome_explicitly_reused=True, actual_compile=False))
                    save()
                record["complete"] = len(record["cases"]) == 258
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException:
        error = traceback.format_exc()
        record["error"] = error
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path.resolve()): digest(path) for path in inputs}
        record["global_inputs_after"] = capture_inputs()
        record["global_references_after"] = references()
        record["inputs_unchanged"] = record["input_hashes_before"] == record["input_hashes_after"]
        record["global_inputs_unchanged"] = record["global_inputs_before"] == record["global_inputs_after"]
        record["global_references_unchanged"] = record["global_references_before"] == record["global_references_after"]
        record["complete"] = (record["complete"] and error is None and record["owned_directory_removed"]
                              and record["inputs_unchanged"] and record["global_inputs_unchanged"]
                              and record["global_references_unchanged"])
        save()
    if error:
        print(error, flush=True)
    require(record["complete"], "Production projection verification failed; retain the diagnostic")
    print("Production projections: 203 Java tests, 258 complete SDKs and 258 compiler outcomes explicitly reused")


if __name__ == "__main__":
    main()
