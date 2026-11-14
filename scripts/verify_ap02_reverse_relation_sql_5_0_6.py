#!/usr/bin/env python3
"""Execute source-derived reverse-relation types through generated SQLite models."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from xml.sax.saxutils import escape

from verify_generated_privacy_metadata import LAUNCHER
from verify_audit_metadata_source_5_0_6 import identities

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same

PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
ORIGINAL = ROOT / ".reference/teaql_code_gen"
RUNTIME = ROOT / ".reference/teaql-rs"
PROJECT = ROOT / "tests/parity/rust_container_metadata_reference_5_0_6"
CORPUS = (PRODUCER / "generator/src/test/resources/"
          "cpp_reverse_relation_type_composition_metadata_5_0_6.json.gz")
RUST = ROOT / "tests/parity/ap02_reverse_relation_sql_5_0_6.rs"
CPP = ROOT / "tests/parity/ap02_reverse_relation_sql_5_0_6.cpp"
SQL = ROOT / "tests/parity/ap02_reverse_relation_sql_5_0_6.sql"
PACKAGES = ("teaql-core", "teaql-macros", "teaql-runtime", "teaql-data-service",
            "teaql-sql", "teaql-provider-sqlite")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(path):
    return {str(item.relative_to(path)): digest(item)
            for item in sorted(path.rglob("*")) if item.is_file()}


def selected_cases():
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    source = next(row for row in corpus["cases"] if row["name"] == "type_composition_0086")
    require(source["input"] == '17, reverse_relation(name="links",target="Child",'
            'json_type="Self"), entity="After", audit_value_max_len=21',
            "Exact retained Self type input")
    base = source["input"].replace(', entity="After"', '').replace(
        'target="Child",', 'target="Child",foreign_key="owner",')
    return corpus, source, [
        {"name": "self_one", "many": False, "input": base},
        {"name": "child_one", "many": False,
         "input": base.replace('json_type="Self"', 'json_type="crate::Child"')},
        {"name": "child_many", "many": True,
         "input": base.replace('json_type="Self"', 'many,json_type="crate::Child"')},
    ]


def model(attribute):
    return ('<root name="ap02_reverse_relation_consumer" data_service="sqlite">'
            '<platform name="string()"/>'
            '<item platform="platform(contextRoot=true)" name="string(column=display_name)" '
            'note="string()?" _table="business_items" _audit_value_max_len="'
            + escape(attribute, {'"': '&quot;'}) + '"/>'
            '<child _parent="item" secret="string(column=password)" owner="integer()" '
            '_table="business_children"/></root>')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", required=True, type=Path)
    args = parser.parse_args()
    require(not args.record.exists() and args.record.suffix == ".gz",
            "Use a fresh compressed execution receipt")
    require(sys.flags.optimize == 0, "Run archive/build recipes with Python assertions enabled")
    corpus, source, cases = selected_cases()
    runtime_commit = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    generator_commit = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())[
        "generator_baseline_commit"]
    require(corpus["runtime_commit"] == runtime_commit
            and corpus["generator_commit"] == generator_commit, "Locked corpus references")
    java_sources = {
        label: sorted(path for package in ("model", "generator")
                      for path in (repository / package / "src/main/java").rglob("*.java"))
        for label, repository in (("original", ORIGINAL), ("cpp", PRODUCER))
    }
    libraries = [ROOT / "build_native_current" / name / ("lib" + name + ".a")
                 for name in ("teaql_runtime", "teaql_sql", "teaql_data_service",
                              "teaql_core", "teaql_memory", "teaql_provider_sqlite")]
    native_headers = sorted(path for package in ROOT.glob("teaql_*")
                            for suffix in ("*.hpp", "*.h")
                            for path in (package / "include").rglob(suffix))
    inputs = [Path(__file__).resolve(), RUST, CPP, SQL, CORPUS,
              ROOT / "upstream.lock.json", ROOT / "toolchains.lock.json",
              PRODUCER / "cpp_generator.lock.json", RUNTIME / "Cargo.lock",
              RUNTIME / "teaql-macros/src/attr.rs",
              RUNTIME / "teaql-macros/src/derive_impl.rs",
              ROOT / "scripts/verify_generated_privacy_metadata.py",
              ROOT / "scripts/verify_audit_metadata_source_5_0_6.py",
              ROOT / "tests/parity/compare_values.py",
              ROOT / "tests/generator/ContainerMetadataProbe.java",
              PROJECT / "Cargo.toml", PROJECT / "Cargo.lock", PROJECT / "main.rs",
              ROOT / "build_native_current/compile_commands.json",
              *native_headers, *libraries, *java_sources["original"], *java_sources["cpp"]]
    before = {str(path): digest(path) for path in inputs}
    record = {"complete": False, "whole_goal_complete": False,
              "scope": "Source-derived reverse-relation types through fresh generated "
                       "runtime assembly, SQLite relation loading, E reads and JSON; finite matrix",
              "runtime_commit": runtime_commit, "generator_commit": generator_commit,
              "source_case": source, "source_corpus_sha256": digest(CORPUS),
              "native_headers": [str(path.relative_to(ROOT)) for path in native_headers],
              "input_hashes_before": before, "free_bytes_before": shutil.disk_usage(ROOT).free,
              "steps": [], "cases": []}

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + "\n").encode(), mtime=0))

    def run(label, command, *, cwd=ROOT, env=None, input_text=None):
        print(label, "running", flush=True)
        result = subprocess.run(list(map(str, command)), cwd=cwd, env=env,
                                input=input_text, text=True, capture_output=True, timeout=900)
        record["steps"].append({"label": label, "command": list(map(str, command)),
                                "cwd": str(cwd), "exit_code": result.returncode,
                                "stdout": result.stdout, "stderr": result.stderr})
        save()
        print(label, "exit", result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    owned = None
    try:
        require(shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "Free space for owned probes")
        run("upstream", ["python3", "scripts/verify_upstream.py"])
        require(subprocess.check_output(["git", "-C", str(ORIGINAL), "rev-parse", "HEAD"],
                                        text=True).strip() == generator_commit,
                "Locked original generator checkout")
        require(not subprocess.check_output(["git", "-C", str(ORIGINAL), "status", "--porcelain"]),
                "Clean original generator checkout")
        with tempfile.TemporaryDirectory(prefix="teaql_ap02_reverse_relation_sql_506_") as name:
            owned = Path(name)
            record["owned_directory"] = name
            env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER="/usr/bin/cc",
                       TEAQL_LOG_ENDPOINT="off")
            reference_env = dict(env, CARGO_TARGET_DIR=str(owned / "metadata_target"))
            run("source_build", ["cargo", "build", "--offline", "--locked", "--manifest-path",
                                  PROJECT / "Cargo.toml", "-j", "2"], env=reference_env)
            require(identities(PROJECT / "Cargo.lock") <= identities(RUNTIME / "Cargo.lock"),
                    "Original metadata dependency identities")
            payload = "".join(json.dumps(row["input"]) + "\n" for row in cases)
            original_metadata = json_lines(run("source_metadata", [
                owned / "metadata_target/debug/teaql_container_metadata_reference_506"],
                env=reference_env, input_text=payload))
            classpaths = {}
            for label, repository in (("original", ORIGINAL), ("cpp", PRODUCER)):
                cpfile = owned / (label + "_classpath.txt")
                run(label + "_classpath", ["mvn", "-q", "-o", "-pl", "generator",
                    "dependency:build-classpath", "-Dmdep.outputFile=" + str(cpfile)], cwd=repository)
                cp = ":".join(str(repository / path) for path in
                              ("generator/target/classes", "model/target/classes"))
                cp += ":" + cpfile.read_text().strip()
                classes = owned / (label + "_classes")
                classes.mkdir()
                extras = []
                if label == "original":
                    launcher = owned / "FrozenPrivacy.java"
                    launcher.write_text(LAUNCHER)
                    extras.append(launcher)
                else:
                    extras.append(ROOT / "tests/generator/ContainerMetadataProbe.java")
                run(label + "_java_build", ["javac", "--release", "17", "-cp", cp,
                    "-d", classes, *java_sources[label], *extras])
                classpaths[label] = ":".join(map(str, [classes,
                    repository / "generator/src/main/resources",
                    repository / "model/src/main/resources"])) + ":" + cp
            java_metadata = json_lines(run("java_metadata", ["java", "-cp", classpaths["cpp"],
                "com.skynet.codegenerator.cpp.ContainerMetadataProbe"], input_text=payload))
            require(len(original_metadata) == len(java_metadata) == len(cases),
                    "Complete original/current metadata cardinality")
            require(all(same(a, b) for a, b in zip(original_metadata, java_metadata, strict=True)),
                    "Original/current metadata effects")
            env["CARGO_TARGET_DIR"] = str(owned / "consumer_target")
            record["rust_environment"] = {key: value for key, value in env.items()
                                           if key.startswith("CARGO_") or key == "TEAQL_LOG_ENDPOINT"}
            commands = json.loads((ROOT / "build_native_current/compile_commands.json").read_text())
            entry = next(row for row in commands
                         if row["file"].endswith("/tests/generator/generated_api_tests.cpp"))
            compiler = list(entry.get("arguments") or shlex.split(entry["command"]))
            for flag in ("-MD", "-MT", "-MF", "-o"):
                while flag in compiler:
                    index = compiler.index(flag)
                    del compiler[index:index + (1 if flag == "-MD" else 2)]
            compiler.remove("-c")
            compiler.remove(entry["file"])
            record["native_compiler"] = {"path": compiler[0], "sha256": digest(Path(compiler[0]))}
            for index, case in enumerate(cases):
                directory = owned / case["name"]
                directory.mkdir()
                xml = directory / "model.xml"
                xml.write_text(model(case["input"]))
                original, cpp = directory / "original", directory / "cpp"
                run(case["name"] + "_original_generate", ["java", "-cp", classpaths["original"],
                    "FrozenPrivacy", xml, original])
                run(case["name"] + "_cpp_generate", ["java", "-cp", classpaths["cpp"],
                    "com.skynet.codegenerator.cpp.CppCoreGenerator", xml, cpp])
                run(case["name"] + "_cpp_format", [ROOT / ".tools/clang-format/bin/clang-format",
                    "--style=file:" + str(ROOT / ".clang-format"), "--Werror", "-i",
                    *sorted(cpp.rglob("*.hpp"))])
                original_files, cpp_files = files(original), files(cpp)
                binary = owned / "native_probe"
                run(case["name"] + "_native_build", [*compiler, "-I" + str(cpp / "include"),
                    '-DTEAQL_AP02_SQL_FIXTURE="' + str(SQL) + '"', CPP, "-Wl,--start-group",
                    *libraries, "-Wl,--end-group",
                    ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a",
                    "-pthread", "-ldl", "-lm", "-o", binary])
                harness = directory / "harness"
                harness.mkdir()
                package = re.search(r'^name\s*=\s*"([^"]+)"',
                                    (original / "lib/Cargo.toml").read_text(), re.M)[1]
                manifest = ('[package]\nname="ap02_reverse_relation_sql_probe"\n'
                            'version="0.1.0"\nedition="2024"\n[workspace]\n'
                            '[[bin]]\nname="ap02_reverse_relation_sql_probe"\npath="'
                            + str(RUST) + '"\n[dependencies]\n'
                            'domain={package="' + package + '",path="' + str(original / "lib") + '"}\n'
                            'serde_json="=1.0.150"\nrusqlite="=0.40.1"\n'
                            'teaql-core="5.0.6"\nteaql-runtime="5.0.6"\n'
                            '[patch.crates-io]\n' + "".join(
                                name + '={path="' + str(RUNTIME / name) + '"}\n' for name in PACKAGES))
                (harness / "Cargo.toml").write_text(manifest)
                shutil.copyfile(RUNTIME / "Cargo.lock", harness / "Cargo.lock")
                run(case["name"] + "_original_resolve", ["cargo", "metadata", "--offline",
                    "--format-version", "1", "--manifest-path", harness / "Cargo.toml"], env=env)
                require(identities(harness / "Cargo.lock") <= identities(RUNTIME / "Cargo.lock"),
                        "Generated consumer registry identities")
                run(case["name"] + "_original_build", ["cargo", "build", "--offline", "--locked",
                    "--manifest-path", harness / "Cargo.toml", "-j", "2"], env=env)
                queries = [{"name": label, "parent": parent, "selected": selected,
                            "minimal": minimal, "many": case["many"]}
                           for label, parent, selected, minimal in (
                               ("unselected", 10, False, False), ("loaded", 10, True, False),
                               ("empty", 11, True, False), ("minimal", 10, True, True),
                               ("no_parent", 99, True, False))]
                query_payload = "".join(json.dumps(row) + "\n" for row in queries)
                source_output = json_lines(run(case["name"] + "_original_run", [
                    owned / "consumer_target/debug/ap02_reverse_relation_sql_probe"],
                    env=env, input_text=query_payload))
                native_output = json_lines(run(case["name"] + "_native_run", [binary],
                                               env=env, input_text=query_payload))
                require(len(source_output) == len(native_output) == len(queries),
                        "Complete generated runtime result cardinality")
                require(files(original) == original_files and files(cpp) == cpp_files,
                        "Generated artifacts changed during compilation/execution")
                record["cases"].append({**case, "model": xml.read_text(),
                    "original_metadata": original_metadata[index], "java_metadata": java_metadata[index],
                    "original_files": original_files, "cpp_files": cpp_files,
                    "consumer_lock": (harness / "Cargo.lock").read_text(),
                    "results": [{"input": query, "original": a, "cpp": b, "match": same(a, b)}
                                for query, a, b in zip(queries, source_output, native_output, strict=True)]})
                save()
            record["differences"] = [{"model": case["name"], "input": row["input"],
                                      "original": row["original"], "cpp": row["cpp"]}
                                     for case in record["cases"] for row in case["results"]
                                     if not row["match"]]
            record["complete"] = not record["differences"]
            record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*")
                                                if path.is_file())
    except Exception as error:
        record["controller_error"] = {"type": type(error).__name__, "message": str(error)}
        raise
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path): digest(path) for path in inputs}
        record["inputs_unchanged"] = before == record["input_hashes_after"]
        record["complete"] = record["complete"] and record["inputs_unchanged"]
        save()
    print("Generated reverse-relation SQLite:", len(record["cases"]), "models,",
          sum(len(case["results"]) for case in record["cases"]), "queries,",
          len(record["differences"]), "differences", flush=True)
    return 0 if record["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
