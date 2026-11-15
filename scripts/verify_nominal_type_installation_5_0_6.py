#!/usr/bin/env python3
"""Qualify installed nominal-type generation using the locked compiler witnesses."""

import argparse
import gzip
import hashlib
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


ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
CANDIDATE = EVIDENCE / "ap02_reverse_relation_nominal_type_candidate_complete_5_0_6.json.gz"
CANDIDATE_SHA256 = "0524e025bf2a6ce49ccad07a5c475844cbfe5c2489285459c87bdd35d3ff04bc"
PREVIOUS = EVIDENCE / "ap02_reverse_relation_nominal_type_candidate_5_0_6.json.gz"
SQL = EVIDENCE / "ap02_reverse_relation_nominal_type_candidate_sql_5_0_6.json.gz"
UNIT = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppNominalTypeProjectionTest.java"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def files(directory):
    return {path.relative_to(directory).as_posix(): digest(path)
            for path in sorted(directory.rglob("*")) if path.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    require(not args.record.exists(), "Preserve preceding execution evidence")
    require(shutil.disk_usage(ROOT).free > 1024 ** 3, "Free space for owned compilation")
    require(digest(CANDIDATE) == CANDIDATE_SHA256, "Reviewed final candidate identity")
    candidate, previous, sql = read(CANDIDATE), read(PREVIOUS), read(SQL)
    require(candidate["complete"] is True and previous["complete"] is True
            and sql["complete"] is True and candidate["installed"] is False,
            "Complete original/candidate witnesses")
    require(candidate["prior_candidate_sha256"] == digest(PREVIOUS)
            and candidate["prior_sql_sha256"] == digest(SQL), "Exact preceding witnesses")
    for name, row in candidate["candidate_overlays"].items():
        require(digest(Path(name)) == row["after_sha256"],
                "Installed production source differs from reviewed candidate: " + name)

    cases = [{"name": row["name"], "model": row["model"],
              "json_type": row["json_type"], "original_exit_code": row["original_rust_exit_code"],
              "files": row["generated_files"]} for row in previous["cases"]]
    cases.extend({"name": row["name"], "model": row["model"],
                  "json_type": row["json_type"], "original_exit_code": row["original_exit_code"],
                  "files": row["results"]["candidate"]["generated_files"]}
                 for row in candidate["cases"])
    require(len(cases) == len({row["name"] for row in cases}) == 22,
            "All twenty-two actual original compiler outcomes")
    sources = sorted(path for package in ("model", "generator")
                     for path in (PRODUCER / package / "src/main/java").rglob("*.java"))
    tests = [PRODUCER / "generator/src/test/java/com/skynet/codegenerator" / name
             for name in TESTS] + [UNIT]
    inputs = [Path(__file__), CANDIDATE, PREVIOUS, SQL, ROOT / "upstream.lock.json",
              ROOT / "toolchains.lock.json", PRODUCER / "cpp_generator.lock.json",
              ROOT / "scripts/verify_audit_metadata_source_5_0_6.py",
              ROOT / "build_native_current/compile_commands.json", *sources, *tests]
    before = {str(path.resolve()): digest(path) for path in inputs}
    runtime_inputs = {name.removeprefix("runtime:"): value
                      for name, value in sql["input_hashes_before"].items()
                      if name.startswith("runtime:teaql_")
                      and name.endswith((".cpp", ".hpp", ".h", ".c", ".cppm"))}
    for name, wanted in runtime_inputs.items():
        require(digest(ROOT / name) == wanted, "Runtime source changed before SQL reuse: " + name)
    for name, wanted in sql["libraries"].items():
        require(digest(Path(name)) == wanted, "Linked SQL library changed: " + name)
    record = {"complete": False, "whole_goal_complete": False, "installed": True,
              "scope": "Production Java, actual generated C++ compiler contracts and exact SQL SDK reuse",
              "runtime_commit": candidate["references_before"]["teaql-rs"],
              "generator_commit": candidate["references_before"]["teaql_code_gen"],
              "candidate": str(CANDIDATE.relative_to(ROOT)), "candidate_sha256": digest(CANDIDATE),
              "input_hashes_before": before, "runtime_cpp_inputs": runtime_inputs,
              "sql_libraries": sql["libraries"], "steps": [], "cases": [], "sql_reproductions": []}

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
            os.killpg(child.pid, signal.SIGTERM)
            try:
                child.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.communicate()
            raise
        result = {"label": label, "command": command, "cwd": str(cwd),
                  "exit_code": child.returncode, "stdout": stdout, "stderr": stderr,
                  "elapsed_seconds": time.monotonic() - started}
        record["steps"].append(result)
        save()
        require(not must_pass or child.returncode == 0,
                label + " failed: " + stderr[-1500:] + stdout[-1500:])
        return result

    owned = None
    error = None
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_nominal_type_installed_506_") as name:
            owned = Path(name)
            record["owned_directory"] = name
            run("producer_package", ["mvn", "-q", "-o", "-pl", "generator", "-am",
                                     "-DskipTests", "package"], cwd=PRODUCER)
            cpfile = owned / "classpath.txt"
            run("classpath", ["mvn", "-q", "-o", "-pl", "generator", "dependency:build-classpath",
                              "-Dmdep.outputFile=" + str(cpfile)], cwd=PRODUCER)
            cp = ":".join(map(str, [PRODUCER / "generator/target/classes",
                                   PRODUCER / "model/target/classes"])) + ":" + cpfile.read_text().strip()
            record["dependency_jars"] = {name: digest(Path(name)) for name in cp.split(":")
                                         if Path(name).is_file()}
            classes = owned / "classes"
            classes.mkdir()
            run("fresh_java_compile", ["javac", "--release", "17", "-cp", cp,
                                       "-d", classes, *sources, *tests])
            cp = ":".join(map(str, [classes, PRODUCER / "generator/src/main/resources",
                                   PRODUCER / "generator/src/test/resources"])) + ":" + cp
            temporary = owned / "java_tmp"
            temporary.mkdir()
            names = ["com.skynet.codegenerator." + name[:-5].replace("/", ".") for name in TESTS]
            names.append("com.skynet.codegenerator.cpp.CppNominalTypeProjectionTest")
            junit = run("junit", ["java", "-Djava.io.tmpdir=" + str(temporary), "-cp", cp,
                                  "org.junit.runner.JUnitCore", *names], cwd=PRODUCER / "generator")
            match = re.search(r"OK \((\d+) tests?\)", junit["stdout"])
            require(match is not None and int(match[1]) == 163, "Complete selected Java regression set")
            record["junit_tests"] = int(match[1])

            entry = next(row for row in json.loads((ROOT / "build_native_current/compile_commands.json").read_text())
                         if row["file"].endswith("/generated_api_tests.cpp"))
            compiler = list(entry.get("arguments") or shlex.split(entry["command"]))
            for flag in ("-MD", "-MT", "-MF", "-o"):
                while flag in compiler:
                    index = compiler.index(flag)
                    del compiler[index:index + (1 if flag == "-MD" else 2)]
            compiler.remove("-c")
            compiler.remove(entry["file"])
            require(digest(Path(compiler[0])) == sql["native_compiler"]["sha256"], "Current native compiler")
            record["native_compiler"] = sql["native_compiler"]
            for case in cases:
                directory = owned / case["name"]
                directory.mkdir()
                xml, output = directory / "model.xml", directory / "cpp"
                xml.write_text(case["model"])
                run(case["name"] + "_generate", ["java", "-cp", cp,
                    "com.skynet.codegenerator.cpp.CppCoreGenerator", xml, output])
                generated = files(output)
                require(generated == case["files"], "Production generation differs from candidate: " + case["name"])
                header = next(output.glob("include/*/models.hpp"))
                source = directory / "consumer.cpp"
                source.write_text('#include "' + str(header) + '"\nint main() { return 0; }\n')
                compiled = run(case["name"] + "_compile", [*compiler, "-I" + str(output / "include"),
                                "-fsyntax-only", source], must_pass=False)
                valid = case["original_exit_code"] == 0
                require((compiled["exit_code"] == 0) == valid
                        and (valid or compiled["exit_code"] > 0 and "error:" in compiled["stderr"]),
                        "Original/installed compiler outcome differs: " + case["name"])
                require(files(output) == generated, "Compiler altered generated files")
                record["cases"].append({**case, "installed_exit_code": compiled["exit_code"], "match": True})
                save()
            for case in sql["cases"]:
                directory = owned / ("sql_" + case["name"])
                directory.mkdir()
                xml, output = directory / "model.xml", directory / "cpp"
                xml.write_text(case["model"])
                run("sql_" + case["name"] + "_generate", ["java", "-cp", cp,
                    "com.skynet.codegenerator.cpp.CppCoreGenerator", xml, output])
                run("sql_" + case["name"] + "_format", [ROOT / ".tools/clang-format/bin/clang-format",
                    "--style=file:" + str(ROOT / ".clang-format"), "--Werror", "-i", *sorted(output.rglob("*.hpp"))])
                generated = files(output)
                require(generated == case["candidate_files"], "SQL SDK changed: " + case["name"])
                record["sql_reproductions"].append({"name": case["name"], "files": generated,
                                                     "reused_results": case["results"]})
            record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
            record["complete"] = len(record["cases"]) == 22 and len(record["sql_reproductions"]) == 3
    except BaseException:
        error = traceback.format_exc()
        record["error"] = error
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path.resolve()): digest(path) for path in inputs}
        record["inputs_unchanged"] = record["input_hashes_after"] == before
        record["runtime_cpp_unchanged"] = all(digest(ROOT / name) == value for name, value in runtime_inputs.items())
        record["sql_libraries_unchanged"] = all(digest(Path(name)) == value for name, value in sql["libraries"].items())
        record["complete"] = (record["complete"] and record["owned_directory_removed"] and record["inputs_unchanged"]
                              and record["runtime_cpp_unchanged"] and record["sql_libraries_unchanged"])
        save()
    if error:
        print(error, flush=True)
    require(record["complete"], "Installed nominal-type verification failed; inspect retained receipt")
    print("Installed nominal types: 163 Java tests, 22 C++ compiler outcomes, 15 explicitly reused SQLite results")


if __name__ == "__main__":
    main()
