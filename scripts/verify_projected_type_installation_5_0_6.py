#!/usr/bin/env python3
"""Verify production generation for the reviewed associated entity-type repair."""

import argparse
import gzip
import json
import os
from pathlib import Path
import re
import resource
import shlex
import shutil
import signal
import subprocess
import tempfile
import time
import traceback

from verify_audit_metadata_source_5_0_6 import TESTS
from verify_nominal_type_installation_5_0_6 import digest, files, read, require


ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
CANDIDATE = EVIDENCE / "ap02_qualified_entity_types_candidate_5_0_6.json.gz"
CANDIDATE_SHA256 = "324991cc1ad2585050a42d75a7d6518dbf6eaf2b18a5979909684aae311b8e83"
DIAGNOSTIC = EVIDENCE / "ap02_qualified_entity_types_diagnostic_5_0_6.json.gz"
DIAGNOSTIC_SHA256 = "47c3962709dffba53c5b0b3ce973bebe8936793842df1a87c6c9d947e1224a16"
NOMINAL = EVIDENCE / "ap02_reverse_relation_nominal_type_installed_5_0_6.json.gz"
NOMINAL_SHA256 = "2aeaa2b94701b615e6b3c91a25b61834269df0396f7c5ed3095175e25c9a380a"
UNIT_DIRECTORY = PRODUCER / "generator/src/test/java/com/skynet/codegenerator"


def bound(path, expected):
    require(digest(path) == expected, "Immutable source/compiler evidence: " + path.name)
    return read(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    require(not args.record.exists(), "Preserve preceding execution receipts")
    require(shutil.disk_usage(ROOT).free > 1024 ** 3, "Owned Java/compiler space")
    candidate = bound(CANDIDATE, CANDIDATE_SHA256)
    diagnostic = bound(DIAGNOSTIC, DIAGNOSTIC_SHA256)
    nominal = bound(NOMINAL, NOMINAL_SHA256)
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and diagnostic["complete"] is True
            and nominal["complete"] is True, "Complete reviewed compiler witnesses")
    for name, row in candidate["overlays"].items():
        require(digest(Path(name)) == row["after_sha256"], "Exact reviewed production Java")
    unit = UNIT_DIRECTORY / "cpp/CppQualifiedEntityTypeProjectionTest.java"
    require(digest(unit) == candidate["candidate_unit_sha256"], "Exact permanent regression source")
    originals = {"qualified_" + row["name"]: row["original_exit_code"]
                 for row in diagnostic["cases"]}
    originals.update({"nominal_" + row["name"]: row["original_exit_code"]
                      for row in nominal["cases"]})
    cases = candidate["cases"]
    require(len(originals) == len(cases) == len({row["name"] for row in cases}) == 30
            and {row["name"] for row in cases} == set(originals), "Complete original compiler matrix")
    for row in cases:
        require(type(row["expected_accept"]) is bool
                and row["expected_accept"] == (originals[row["name"]] == 0), "Actual source outcome")

    sources = sorted(path for package in ("model", "generator")
                     for path in (PRODUCER / package / "src/main/java").rglob("*.java"))
    tests = [UNIT_DIRECTORY / name for name in TESTS]
    tests.extend([UNIT_DIRECTORY / "cpp/CppNominalTypeProjectionTest.java", unit])
    headers = sorted(path for package in ROOT.glob("teaql_*")
                     for suffix in ("*.hpp", "*.h") for path in (package / "include").rglob(suffix))
    inputs = [Path(__file__), ROOT / "scripts/verify_nominal_type_installation_5_0_6.py",
              ROOT / "scripts/verify_audit_metadata_source_5_0_6.py", CANDIDATE, DIAGNOSTIC, NOMINAL,
              ROOT / "upstream.lock.json", ROOT / "toolchains.lock.json",
              PRODUCER / "cpp_generator.lock.json", ROOT / "build_native_current/compile_commands.json",
              *sources, *tests, *headers]
    before = {str(path.resolve()): digest(path) for path in inputs}
    jars = candidate["dependency_jars"]
    require(all(digest(Path(name)) == value for name, value in jars.items()), "Unchanged Java dependencies")
    record = {"complete": False, "installed": True, "whole_goal_complete": False,
              "scope": "Fresh production Java and complete generated C++ compiler contracts; original outcomes explicitly reused, SQL has a separate production gate",
              "candidate_sha256": CANDIDATE_SHA256, "diagnostic_sha256": DIAGNOSTIC_SHA256,
              "nominal_sha256": NOMINAL_SHA256, "input_hashes_before": before,
              "dependency_jars": jars, "steps": [], "cases": [],
              "option_model_exception": "pending_user_choice"}

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))

    def run(label, command, *, cwd=ROOT, must_pass=True):
        command = list(map(str, command))
        print(label + " running", flush=True)
        started = time.monotonic()
        child = subprocess.Popen(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, start_new_session=True)
        try:
            stdout, stderr = child.communicate(timeout=900)
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
        result = {"label": label, "command": command, "cwd": str(cwd),
                  "exit_code": child.returncode, "stdout": stdout, "stderr": stderr,
                  "duration_seconds": time.monotonic() - started}
        record["steps"].append(result)
        save()
        require(not must_pass or child.returncode == 0, label + " failed: " + stderr[-1500:])
        return result

    owned = None
    error = None
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_projected_types_installed_506_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                run("producer_package", ["mvn", "-q", "-o", "-pl", "generator", "-am",
                                         "-DskipTests", "package"], cwd=PRODUCER)
                cp = ":".join(jars)
                classes = owned / "classes"
                classes.mkdir()
                run("production_fresh_java_compile", ["javac", "--release", "17", "-cp", cp,
                                                       "-d", classes, *sources, *tests])
                cp = ":".join(map(str, [classes, PRODUCER / "generator/src/main/resources",
                                       PRODUCER / "model/src/main/resources",
                                       PRODUCER / "generator/src/test/resources"])) + ":" + cp
                names = ["com.skynet.codegenerator." + name[:-5].replace("/", ".") for name in TESTS]
                names.extend(["com.skynet.codegenerator.cpp.CppNominalTypeProjectionTest",
                              "com.skynet.codegenerator.cpp.CppQualifiedEntityTypeProjectionTest"])
                junit = run("production_junit", ["java", "-Djava.io.tmpdir=" + directory,
                            "-cp", cp, "org.junit.runner.JUnitCore", *names], cwd=PRODUCER / "generator")
                match = re.search(r"OK \((\d+) tests?\)", junit["stdout"])
                require(match is not None and int(match[1]) == 167, "All 167 permanent Java tests")
                record["junit_tests"] = 167
                entry = next(row for row in json.loads((ROOT / "build_native_current/compile_commands.json").read_text())
                             if row["file"].endswith("/generated_api_tests.cpp"))
                compiler = list(entry.get("arguments") or shlex.split(entry["command"]))
                for flag in ("-MD", "-MT", "-MF", "-o"):
                    while flag in compiler:
                        index = compiler.index(flag)
                        del compiler[index:index + (1 if flag == "-MD" else 2)]
                compiler.remove("-c")
                compiler.remove(entry["file"])
                record["native_compiler"] = {"path": compiler[0], "sha256": digest(Path(compiler[0]))}
                require(record["native_compiler"] == candidate["native_compiler"], "Qualified native compiler")
                for case in cases:
                    directory = owned / case["name"]
                    directory.mkdir()
                    xml, output = directory / "model.xml", directory / "cpp"
                    xml.write_text(case["model"])
                    run(case["name"] + "_generate", ["java", "-cp", cp,
                        "com.skynet.codegenerator.cpp.CppCoreGenerator", xml, output])
                    generated = files(output)
                    require(generated == case["generated_files"], "Production/candidate SDK equality")
                    header = next(output.glob("include/*/models.hpp"))
                    source = directory / "consumer.cpp"
                    source.write_text('#include "' + str(header) + '"\nint main() { return 0; }\n')
                    result = run(case["name"] + "_compile", [*compiler, "-I" + str(output / "include"),
                                 "-fsyntax-only", source], must_pass=False)
                    require((result["exit_code"] == 0) == case["expected_accept"]
                            and (result["exit_code"] == 0 or result["exit_code"] > 0
                                 and "error:" in result["stderr"]), "Actual original/production compiler outcome")
                    require(files(output) == generated, "Compiler preserves complete SDK")
                    record["cases"].append({"name": case["name"], "model": case["model"],
                        "type": case["type"], "original_exit_code": originals[case["name"]],
                        "installed_exit_code": result["exit_code"], "match": True, "files": generated})
                    save()
                record["complete"] = len(record["cases"]) == 30
            finally:
                record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
    except BaseException:
        error = traceback.format_exc()
        record["error"] = error
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path.resolve()): digest(path) for path in inputs}
        record["inputs_unchanged"] = record["input_hashes_after"] == before
        record["complete"] = record["complete"] and error is None and record["inputs_unchanged"] and record["owned_directory_removed"]
        save()
    if error:
        print(error, flush=True)
    require(record["complete"], "Production type qualification failed; inspect retained receipt")
    print("Production projected types: 167 Java tests and 30 original/installed compiler outcomes")


if __name__ == "__main__":
    main()
