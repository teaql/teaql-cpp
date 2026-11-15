#!/usr/bin/env python3
"""Qualify installed primitive constants, tuple values and preserved SDK contracts."""

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
        "ap02_primitive_const_path_candidate_v2_5_0_6.json.gz",
        "58a5af8a15b873712640479744408adc6ea2d5af699e452b3838d8fd5f81be96"
    ],
    "preceding": [
        "ap02_scalar_const_value_installed_5_0_6.json.gz",
        "c194c580dac6a0922084897b6e7df1599ea681d514f9b33e01403920aa535625"
    ],
    "models": [
        "ap02_primitive_const_path_model_inventory_5_0_6.json.gz",
        "2001fe75959d703b020d882932050adc4e22f762405a700d2c4c508d9d00e564"
    ],
    "expressions": [
        "ap02_primitive_const_expression_matrix_v3_5_0_6.json.gz",
        "7d1696a7c54c5073cc90a2e8027af9dd1401589c4b942eefd828b79677641103"
    ],
    "language": [
        "ap02_primitive_const_path_language_matrix_v2_5_0_6.json.gz",
        "c90e193636099755ad6fd3ee07b33727ef80d165676531df50d4f14c9608a145"
    ],
    "proposal": [
        "ap02_primitive_const_path_proposal_5_0_6.json.gz",
        "5297e6b21608e7ef8e261ee43e576d46b2dd6ca0b8596ef2d8c004b2b2372ad7"
    ],
    "controls": [
        "ap02_primitive_const_path_copied_controls_5_0_6.json.gz",
        "ebff176e545a002ddb034cfcb8d075694a61796404254115740501de1e4d6ab2"
    ]
}
UNIT_NAMES = (
    "CppNominalTypeProjectionTest", "CppQualifiedEntityTypeProjectionTest",
    "CppContainerTypeProjectionTest", "CppResultTypeProjectionTest",
    "CppStandardValueTypeProjectionTest", "CppSmartListTypeProjectionTest",
    "CppArrayConstExpressionProjectionTest", "CppScalarConstValueProjectionTest",
    "CppPrimitiveConstPathProjectionTest",
)


def bound(key):
    name, expected = RECORDS[key]
    path = EVIDENCE / name
    require(digest(path) == expected, "Immutable compiler witness: " + name)
    return read(path)


def selected_cases(candidate, preceding):
    models, expressions = bound("models"), bound("expressions")
    language, proposal, controls = bound("language"), bound("proposal"), bound("controls")
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["junit_tests"] == 213
            and candidate["differences"] == [] and len(candidate["cases"]) == 61
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["owned_directory_removed"] is True
            and candidate["actual_cpp_compiler_processes"] == 61
            and candidate["reused_original_compiler_outcomes"] == 61
            and candidate["new_original_compiler_processes"] == 0,
            "Complete copied primitive/tuple compiler and permanent regression qualification")
    require(models["complete"] is True and len(models["cases"]) == 61
            and expressions["complete"] is True and len(expressions["cases"]) == 928
            and language["complete"] is True and len(language["constant_inventory"]) == 70
            and len(language["canonical"]) == 560 and len(language["legacy"]) == 280
            and expressions["language_sha256"] == RECORDS["language"][1]
            and candidate["diagnostic_sha256"] == RECORDS["models"][1]
            and candidate["expression_matrix_sha256"] == RECORDS["expressions"][1],
            "Complete actual catalog, expression contexts and original compiler owners")
    require(proposal["complete"] is True and proposal["apply_check_exit_code"] == 0
            and proposal["candidate_sha256"] == RECORDS["candidate"][1]
            and len(proposal["paths"]) == 3
            and controls["complete"] is True and len(controls["modes"]) == 2
            and {row["optimized"] for row in controls["modes"]} == {0, 1}
            and all(len(row["controls"]) == 15 and all(item["rejected"] is True
                    for item in row["controls"]) for row in controls["modes"]),
            "Reviewable exact patch and actual dual-mode copied-evidence controls")
    require(preceding["complete"] is True and preceding["installed"] is True
            and preceding["junit_tests"] == 203 and len(preceding["cases"]) == 258,
            "All preceding production SDK/compiler contracts")
    original = {row["name"]: row for row in models["cases"]}
    require({row["name"] for row in candidate["cases"]} == set(original),
            "Every copied compiler case has a completed original owner")
    for row in candidate["cases"]:
        prior = original[row["name"]]
        require(row["original_compiler_outcome_explicitly_reused"] is True and row["match"] is True
                and all(row[key] == prior[key] for key in (
                    "model", "type", "original_files", "original_exit_code",
                    "consumer_source", "consumer_manifest", "consumer_lock"))
                and type(row["cpp_exit_code"]) is int and type(row["original_exit_code"]) is int
                and (row["cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                and (row["cpp_exit_code"] != 0
                     or len(row["cpp_projection"]) == row["original_projection_count"]),
                "Exact SDK/consumer identity and actual compiled projection")
    require(sum(row["cpp_exit_code"] == 0 for row in candidate["cases"]) == 52
            and sum(row["original_projection_count"] == 1 for row in candidate["cases"]) == 50,
            "Compiler acceptance and retained projection remain distinct")
    result = []
    for prefix, owner, rows, file_key, code_key in (
        ("preserved_", "preceding", preceding["cases"], "files", "reused_cpp_exit_code"),
        ("primitive_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in rows:
            selected = dict(name=prefix + row["name"], source_case=row["name"],
                            source_record=owner, model=row["model"], type=row["type"],
                            files=row[file_key], original_exit_code=row["original_exit_code"],
                            reused_cpp_exit_code=row[code_key])
            if owner == "candidate":
                selected["expected_projection_count"] = row["original_projection_count"]
            result.append(selected)
    require(len(result) == len({row["name"] for row in result}) == 319,
            "All 258 preserved and 61 primitive/tuple SDK/compiler contracts")
    require(all(type(row["reused_cpp_exit_code"]) is int
                and type(row["original_exit_code"]) is int
                and (row["reused_cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                for row in result), "Strict original/native compiler outcomes")
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
    unit = unit_root / "cpp/CppPrimitiveConstPathProjectionTest.java"
    require(digest(unit) == candidate["candidate_unit_sha256"], "Ten permanent primitive/tuple regression methods")
    fixture_resource = PRODUCER / candidate["candidate_resource"]["path"]
    require(digest(fixture_resource) == candidate["candidate_resource"]["sha256"], "Immutable primitive/tuple fixture")
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
              fixture_resource, *[EVIDENCE / value[0] for value in RECORDS.values()], *sources, *tests, *headers]
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
                  scope="Fresh production Java and 319 complete SDKs; all compiler outcomes explicitly reused through exact whole-SDK equality; zero new C++ compiler processes",
                  candidate_sha256=RECORDS["candidate"][1],
                  preceding_sha256=RECORDS["preceding"][1], input_hashes_before=before,
                  global_inputs_before=global_before, global_references_before=refs_before,
                  dependency_jars=jars, native_compiler=compiler, new_cpp_compiler_processes=0,
                  reused_cpp_compiler_outcomes=319, steps=[], cases=[],
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
        with tempfile.TemporaryDirectory(prefix="teaql_primitive_const_installed_506_") as directory:
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
                require(re.search(r"OK \(213 tests\)", output) is not None,
                        "All 213 permanent production Java tests")
                record["junit_tests"] = 213
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
                record["complete"] = len(record["cases"]) == 319
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
    print("Production projections: 213 Java tests, 319 complete SDKs and 319 compiler outcomes explicitly reused")


if __name__ == "__main__":
    main()
