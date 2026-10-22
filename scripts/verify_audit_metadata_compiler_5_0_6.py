#!/usr/bin/env python3
"""Compile fresh original/C++ SDKs for corrected audit-parser caller branches.

Original entities remain unchanged. Native outputs compare complete descriptor
and field-privacy observations, not SQL execution or whole generator parity.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
from xml.sax.saxutils import escape

from audit_generated_privacy_contracts_5_0_6 import RUST_MAIN, CPP_MAIN
from verify_generated_privacy_metadata import LAUNCHER
from verify_audit_metadata_source_5_0_6 import identities

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / ".reference/teaql_code_gen"
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
CASES = [
    "decimal",
    "plus_pattern_audit_value_max_len",
    "union_bad_generic_audit_value_max_len",
    "alias_after_where_audit_value_max_len",
    "module_local_audit_value_max_len",
    "outer_doc_only",
    "numeric_generic",
    "fn_missing_type_audit_value_max_len",
    "call_bad_qualified",
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != ".gz":
        parser.error("Use a fresh compressed evidence path")
    corpus_path = PRODUCER / "generator/src/test/resources/cpp_audit_metadata_5_0_6.json.gz"
    corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
    selected = [next(case for case in corpus["cases"] if case["name"] == name)
                for name in CASES]
    assert all(case["expected"]["parsed"] for case in selected)
    runtime = ROOT / ".reference/teaql-rs"
    libraries = [ROOT / "build_native_current" / package / ("lib" + package + ".a")
                 for package in ("teaql_runtime", "teaql_sql", "teaql_data_service",
                                 "teaql_core", "teaql_memory", "teaql_provider_sqlite")]
    original_sources = sorted(path for package in ("model", "generator")
                              for path in (ORIGINAL / package / "src/main/java").rglob("*.java"))
    cpp_sources = sorted((PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp")
                         .glob("*.java"))
    inputs = [Path(__file__).resolve(), corpus_path, *original_sources, *cpp_sources,
              ROOT / "scripts/audit_generated_privacy_contracts_5_0_6.py",
              ROOT / "scripts/verify_generated_privacy_metadata.py", *libraries]
    before = {str(path): digest(path) for path in inputs}
    record = {
        "scope": "Fresh complete original generated entities and native SDK "
                 "descriptor/field-privacy callers for nine audit-expression cases; "
                 "no runtime assembly, SQL, full AP02 or complete generator acceptance",
        "complete": False,
        "runtime_commit": corpus["runtime_commit"],
        "generator_commit": corpus["generator_commit"],
        "input_hashes_before": before,
        "free_bytes_before": shutil.disk_usage(ROOT).free,
        "steps": [],
        "cases": [],
    }

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + "\n").encode(), mtime=0))

    def run(label, command, *, cwd=ROOT, env=None):
        print(label, "running", flush=True)
        result = subprocess.run(command, cwd=cwd, env=env, text=True,
                                capture_output=True, timeout=900)
        record["steps"].append({"label": label, "command": list(map(str, command)),
                                "cwd": str(cwd), "exit_code": result.returncode,
                                "stdout": result.stdout, "stderr": result.stderr})
        save()
        print(label, "exit", result.returncode, flush=True)
        result.check_returncode()
        return result

    owned = None
    try:
        assert shutil.disk_usage(ROOT).free > 2 * 1024 ** 3
        run("upstream", ["python3", "scripts/verify_upstream.py"])
        assert subprocess.check_output(["git", "-C", str(ORIGINAL), "rev-parse", "HEAD"],
                                       text=True).strip() == corpus["generator_commit"]
        assert not subprocess.check_output(["git", "-C", str(ORIGINAL), "status",
                                             "--porcelain", "--untracked-files=no"]).strip()
        with tempfile.TemporaryDirectory(prefix="teaql_audit_compiler_506_") as name:
            owned = Path(name)
            record["owned_directory"] = name
            classpaths = {}
            for label, repository, sources in (("original", ORIGINAL, original_sources),
                                                ("cpp", PRODUCER, cpp_sources)):
                classpath_file = owned / (label + "_classpath.txt")
                run(label + "_classpath", ["mvn", "-q", "-o", "-pl", "generator",
                    "dependency:build-classpath", "-Dmdep.outputFile=" + str(classpath_file)],
                    cwd=repository)
                cp = ":".join(str(repository / path) for path in
                              ("generator/target/classes", "model/target/classes"))
                cp += ":" + classpath_file.read_text().strip()
                classes = owned / (label + "_classes")
                classes.mkdir()
                extra = []
                if label == "original":
                    launcher = owned / "FrozenPrivacy.java"
                    launcher.write_text(LAUNCHER)
                    extra.append(str(launcher))
                run(label + "_java_compile", ["javac", "--release", "17", "-cp", cp,
                    "-d", str(classes), *map(str, sources), *extra])
                classpaths[label] = ":".join(map(str, [classes,
                    repository / "generator/src/main/resources",
                    repository / "model/src/main/resources"])) + ":" + cp

            env = dict(os.environ, CARGO_TARGET_DIR=str(owned / "target"),
                       CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER="/usr/bin/cc",
                       TEAQL_LOG_ENDPOINT="off")
            record["rust_environment"] = {key: value for key, value in env.items()
                                          if key.startswith("CARGO_") or key == "TEAQL_LOG_ENDPOINT"}
            includes = []
            invocations = []
            for index, sample in enumerate(selected):
                namespace = "cpp_audit_case_" + str(index)
                directory = owned / str(index)
                directory.mkdir()
                value = escape(sample["input"], {'"': "&quot;", "\n": "&#10;",
                                                 "\r": "&#13;", "\t": "&#9;"})
                model = directory / "model.xml"
                model.write_text(
                    '<root name="' + namespace + '" data_service="sqlite">'
                    '<platform name="string()"/><item platform="platform(contextRoot=true)" '
                    'name="string(column=display_name)" note="string()?" _table="business_items" '
                    '_audit_mask_fields="name,platform" _audit_value_max_len="' + value + '"/>'
                    '<child _parent="item" secret="string(column=password)" '
                    '_table="business_children" _audit_mask_fields="secret"/></root>')
                original = directory / "original"
                cpp = directory / "cpp"
                run(sample["name"] + "_original_generate", ["java", "-cp", classpaths["original"],
                    "FrozenPrivacy", str(model), str(original)])
                metadata = json.loads((original / "metadata.json").read_text())
                assert next(row for row in metadata if row["entity"] == "Item")["audit_limit"] == sample["input"]
                run(sample["name"] + "_cpp_generate", ["java", "-cp", classpaths["cpp"],
                    "com.skynet.codegenerator.cpp.CppCoreGenerator", str(model), str(cpp)])
                run(sample["name"] + "_cpp_format", [str(ROOT / ".tools/clang-format/bin/clang-format"),
                    "--style=file:" + str(ROOT / ".clang-format"), "--Werror", "-i",
                    *map(str, cpp.rglob("*.hpp"))])
                harness = directory / "harness"
                harness.mkdir()
                original_hashes = {str(path.relative_to(original)): digest(path)
                                   for path in original.rglob("*") if path.is_file()}
                imports = "".join('#[path="' + str(original / "lib/src" / entity / "entity.rs")
                                  + '"] mod ' + entity + '_entity; pub use ' + entity + '_entity::*;\n'
                                  for entity in ("platform", "item", "child"))
                (harness / "main.rs").write_text(imports + RUST_MAIN)
                (harness / "Cargo.toml").write_text(
                    '[package]\nname="teaql_audit_compiler_506"\nversion="0.1.0"\nedition="2024"\n'
                    '[workspace]\n[[bin]]\nname="teaql_audit_compiler_506"\npath="main.rs"\n'
                    '[dependencies]\nserde_json="=1.0.150"\nserde="=1.0.228"\n'
                    'chrono="=0.4.45"\nrust_decimal="=1.42.1"\n'
                    + "".join(package + '={path="' + str(runtime / package) + '"}\n'
                              for package in ("teaql-core", "teaql-runtime", "teaql-data-service", "teaql-macros")))
                shutil.copyfile(runtime / "Cargo.lock", harness / "Cargo.lock")
                run(sample["name"] + "_original_build", ["cargo", "build", "--offline",
                    "--manifest-path", str(harness / "Cargo.toml"), "-j", "2"], env=env)
                assert identities(harness / "Cargo.lock") <= identities(runtime / "Cargo.lock")
                source_result = json.loads(run(sample["name"] + "_original_run",
                    [str(owned / "target/debug/teaql_audit_compiler_506")], env=env).stdout)
                assert original_hashes == {str(path.relative_to(original)): digest(path)
                                          for path in original.rglob("*") if path.is_file()}
                files = {str(path.relative_to(directory)): {"sha256": digest(path), "text": path.read_text()}
                         for tree in (original, cpp, harness) for path in tree.rglob("*") if path.is_file()}
                record["cases"].append({"name": sample["name"], "input": sample["input"],
                    "model": model.read_text(), "original": source_result, "files": files})
                includes.append('#include "' + str(cpp / "include" / (namespace + ".hpp")) + '"')
                invocations.append("emit_case<" + namespace + "::Platform, " + namespace + "::Item, "
                                   + namespace + "::Child>(global, first);")
                save()

            observer = CPP_MAIN[CPP_MAIN.index("static Json observe"):CPP_MAIN.index("int main()")]
            source = owned / "observer.cpp"
            source.write_text("\n".join(includes) + "\n#include <iostream>\nusing namespace teaql;\n"
                + observer + '''
template <class Platform, class Item, class Child>
void emit_case(GlobalContext& global, bool& first) {
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    Json::Array rows(r);
    rows.emplace_back(observe(r, Platform::entity_descriptor(r)));
    rows.emplace_back(observe(r, Item::entity_descriptor(r)));
    rows.emplace_back(observe(r, Child::entity_descriptor(r)));
    if (!first) { std::cout << ','; }
    first = false;
    std::cout << Json(r, std::move(rows)).dump(r);
    user_context.reset();
    if (global.live_user_bytes() != 0) { throw std::runtime_error("Context bytes remain"); }
}
int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    bool first = true;
    std::cout << '[';
''' + "\n".join(invocations) + r'''
    std::cout << "]\n";
}
''')
            run("observer_format", [str(ROOT / ".tools/clang-format/bin/clang-format"), "-i",
                "--style=file:" + str(ROOT / ".clang-format"), str(source)])
            record["cpp_main"] = source.read_text()
            entries = json.loads((ROOT / "build_native_current/compile_commands.json").read_text())
            entry = next(row for row in entries if row["file"].endswith("/tests/generator/generated_api_tests.cpp"))
            command = list(entry.get("arguments") or shlex.split(entry["command"]))
            for flag in ("-MD", "-MT", "-MF", "-o"):
                while flag in command:
                    position = command.index(flag)
                    del command[position:position + (1 if flag == "-MD" else 2)]
            command.remove("-c")
            command.remove(entry["file"])
            binary = owned / "observer"
            run("native_build", [*command, str(source), "-Wl,--start-group", *map(str, libraries),
                "-Wl,--end-group", str(ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a"),
                "-pthread", "-ldl", "-lm", "-o", str(binary)])
            observed = json.loads(run("native_run", [str(binary)], env=env).stdout)
            assert len(observed) == len(record["cases"])
            for case, actual in zip(record["cases"], observed):
                case["cpp"] = actual
                case["match"] = case["original"] == actual
            record["differences"] = [case["name"] for case in record["cases"] if not case["match"]]
            record["complete"] = not record["differences"]
            record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*")
                                                if path.is_file())
    except Exception as error:
        record["controller_error"] = {"type": type(error).__name__, "message": str(error)}
        save()
        raise
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path): digest(path) for path in inputs}
        record["inputs_unchanged"] = record["input_hashes_after"] == before
        record["complete"] = record["complete"] and record["inputs_unchanged"]
        save()
    return 0 if record["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
