#!/usr/bin/env python3
"""Qualify installed standard-value and SmartList projection generation."""

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
    "candidate": ("ap02_standard_smart_list_candidate_5_0_6.json.gz",
                  "7913c56de1a85dd347073c6d27d317b7362504a26c3c8df93b38ccae4d4f4d57"),
    "standard": ("ap02_standard_value_type_candidate_5_0_6.json.gz",
                 "b5cf26c7575594d5bbc826309dae2961a48ef51a411a7023eb9c45175dd05a00"),
    "preceding": ("ap02_result_error_types_installed_repaired_5_0_6.json.gz",
                  "a6446037cdac282b5809b5b63eb236cfeb9e9792a50fa9be60394b551b25ff25"),
}
UNIT_NAMES = (
    "CppNominalTypeProjectionTest", "CppQualifiedEntityTypeProjectionTest",
    "CppContainerTypeProjectionTest", "CppResultTypeProjectionTest",
    "CppStandardValueTypeProjectionTest", "CppSmartListTypeProjectionTest",
)


def bound(key):
    name, expected = RECORDS[key]
    path = EVIDENCE / name
    require(digest(path) == expected, "Immutable compiler witness: " + name)
    return read(path)


def selected_cases(candidate, standard, preceding):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["junit_tests"] == 186
            and len(candidate["cases"]) == 21 and len(candidate["standard_artifact_reuse"]) == 34,
            "Complete combined source/compiler qualification")
    require(standard["complete"] is True and standard["parity_passed"] is True
            and standard["junit_tests"] == 183 and len(standard["cases"]) == 34
            and candidate["standard_candidate_sha256"] == RECORDS["standard"][1],
            "Complete standard-value source/compiler qualification")
    require(preceding["complete"] is True and preceding["installed"] is True
            and len(preceding["cases"]) == 84, "Complete preceding production compiler qualification")
    reused = {row["name"]: row for row in candidate["standard_artifact_reuse"]}
    require(len(reused) == 34, "Unique complete standard artifact reuse")
    for row in standard["cases"]:
        actual = reused[row["name"]]
        require(all(actual[key] == row[key] for key in ("name", "model", "type", "cpp_files",
                                                       "original_exit_code", "cpp_exit_code"))
                and actual["compiler_outcomes_explicitly_reused"] is True,
                "Exact generated standard input/artifact/result identity")
    result = []
    for prefix, source, rows, file_key, code_key in (
        ("preserved_", "preceding", preceding["cases"], "files", "installed_exit_code"),
        ("standard_", "standard", standard["cases"], "cpp_files", "cpp_exit_code"),
        ("smart_list_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in rows:
            result.append(dict(name=prefix + row["name"], source_case=row["name"],
                               source_record=source, model=row["model"], type=row["type"],
                               files=row[file_key], original_exit_code=row["original_exit_code"],
                               reused_cpp_exit_code=row[code_key]))
    require(len(result) == len({row["name"] for row in result}) == 139,
            "All 84 preserved, 34 standard-value and 21 SmartList contracts")
    require(sum(row["reused_cpp_exit_code"] == 0 for row in result) == 94
            and all((row["reused_cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                    for row in result), "All 94 accepted and 45 refused outcomes")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    require(not args.record.exists(), "Preserve earlier qualification receipts")
    require(sys.flags.optimize == 0,
            "Run generation with ordinary Python; acceptance predicates have separate dual-mode controls")
    require(shutil.disk_usage(ROOT).free > 1024 ** 3, "Owned production verification space")
    candidate, standard, preceding = (bound(key) for key in RECORDS)
    cases = selected_cases(candidate, standard, preceding)
    for name, row in candidate["candidate_overlays"].items():
        require(digest(Path(name)) == row["after_sha256"], "Exact reviewed installed implementation")
    unit_root = PRODUCER / "generator/src/test/java/com/skynet/codegenerator"
    for name, expected in candidate["candidate_unit_sha256"].items():
        require(digest(unit_root / ("cpp/" + name + ".java")) == expected, "Permanent regression source")
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
              *[EVIDENCE / value[0] for value in RECORDS.values()], *sources, *tests, *headers]
    before = {str(path.resolve()): digest(path) for path in inputs}
    global_before, refs_before = capture_inputs(), references()
    jars = candidate["java_dependencies"]
    require(jars == standard["java_dependencies"] == preceding["dependency_jars"]
            and all(digest(Path(name)) == value for name, value in jars.items()),
            "Identical qualified Java dependency set")
    compiler = candidate["native_compiler"]
    require(compiler == standard["native_compiler"] == preceding["native_compiler"]
            and digest(Path(compiler["path"])) == compiler["sha256"], "Retained actual native compiler")
    record = dict(complete=False, installed=True, whole_goal_complete=False,
                  scope="Fresh production Java and 139 complete SDKs; all compiler outcomes explicitly reused through exact whole-SDK equality; zero new C++ compiler processes",
                  candidate_sha256=RECORDS["candidate"][1], standard_sha256=RECORDS["standard"][1],
                  preceding_sha256=RECORDS["preceding"][1], input_hashes_before=before,
                  global_inputs_before=global_before, global_references_before=refs_before,
                  dependency_jars=jars, native_compiler=compiler, new_cpp_compiler_processes=0,
                  reused_cpp_compiler_outcomes=139, steps=[], cases=[],
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
        with tempfile.TemporaryDirectory(prefix="teaql_standard_smart_list_installed_506_") as directory:
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
                require(re.search(r"OK \(186 tests\)", output) is not None,
                        "All 186 permanent production Java tests")
                record["junit_tests"] = 186
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
                record["complete"] = len(record["cases"]) == 139
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
    print("Production projections: 186 Java tests, 139 complete SDKs and 139 compiler outcomes explicitly reused")


if __name__ == "__main__":
    main()
