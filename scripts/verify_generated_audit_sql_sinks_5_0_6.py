#!/usr/bin/env python3
"""Run source-derived audit metadata through generated SQLite queries and sinks."""

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

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same
from verify_constant_closure_5_0_6 import registry_identities
from verify_dynamic_json_privacy_sinks_5_0_6 import ACK

ORIGINAL = ROOT / ".reference/teaql_code_gen"
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
RUNTIME = ROOT / ".reference/teaql-rs"
MODEL = ROOT / "tests/parity/inheritance_audit_mask_expression_5_0_6.xml"
BASE_MODEL = ROOT / "tests/generator/inheritance_graph.xml"
SOURCE_CASES = (PRODUCER /
                "generator/src/test/resources/cpp_nested_item_checkpoint_metadata_5_0_6.json.gz")
DEFAULT_MODEL_CASES = ROOT / "tests/parity/default_model_literal_final_inputs_5_0_6.json.gz"
RUST_PROBE = ROOT / "tests/parity/rust_dynamic_json_privacy_5_0_6/src/main.rs"
RUST_MANIFEST = ROOT / "tests/parity/rust_dynamic_json_privacy_5_0_6/Cargo.toml"
RUST_LOCK = ROOT / "tests/parity/rust_dynamic_json_privacy_5_0_6/Cargo.lock"
CPP_PROBE = ROOT / "tests/parity/generated_dynamic_json_privacy_probe.cpp"
RUST_METADATA_PROBE = ROOT / "tests/parity/inheritance_audit_metadata_rust_5_0_6.rs"
CPP_METADATA_PROBE = ROOT / "tests/parity/inheritance_audit_metadata_cpp_5_0_6.cpp"
RUST_SAVE_PROBE = ROOT / "tests/parity/inheritance_audit_save_rust_5_0_6.rs"
CPP_SAVE_PROBE = ROOT / "tests/parity/inheritance_audit_save_cpp_5_0_6.cpp"
SINK_VERIFIER = ROOT / "tests/parity/verify_dynamic_json_privacy_sinks_5_0_6.py"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def file_hashes(directory):
    return {str(path.relative_to(directory)): digest(path)
            for path in sorted(directory.rglob("*")) if path.is_file()}


SOURCE_CASE_EXPECTATIONS = {
    "cast_audit_value_max_len_mask": ("17", "PR*****...*****RY", True),
    "sequence_boundary_9": ("19", "PR******...******RY", True),
    "audit_value_max_len_0_then_assignment": ("21", "PR*****************RY", False),
}


def checked_model(case_name, mask_declaration):
    with gzip.open(SOURCE_CASES, "rt") as source:
        corpus = json.load(source)
    case = next(value for value in corpus["cases"]
                if value["name"] == case_name)
    assert case["expected"]["parsed"]
    assert case["expected"]["limit"] == SOURCE_CASE_EXPECTATIONS[case_name][0]
    assert case["input"].startswith("17,")
    if mask_declaration != "named":
        assert case_name == "audit_value_max_len_0_then_assignment"
    if mask_declaration == "partial_unknown":
        with gzip.open(DEFAULT_MODEL_CASES, "rt") as source:
            default_cases = json.load(source)
        partial_case = next(value for value in default_cases["cases"]
                            if value["name"] == "partial_unknown")
        assert partial_case["expected"]["parsed"]
        assert (partial_case["table_source"] ==
                'business", unknown=0, spare="')
        assert ('_table="business&quot;, unknown=0, spare=&quot;" '
                '_audit_mask_fields="name,platform"' in partial_case["model"])
    mask_attribute = {
        "named": ' _audit_mask_fields="name"',
        "empty": ' _audit_mask_fields=""',
        "absent": "",
        "credential_field": ' password="string()" _audit_mask_fields="name"',
        "partial_unknown": (
            ' _table="school_data&quot;, data_service=&quot;sqlite&quot;, '
            'unknown=0, spare=&quot;" _audit_mask_fields="name"'),
    }[mask_declaration]
    old = 'advisor="specialist(optional=true)" _audit_mask_fields="name" />'
    new = ('advisor="specialist(optional=true)"' + mask_attribute + '\n'
           '            _audit_value_max_len="' + escape(case["input"], {'"': '&quot;'}) + '" />')
    baseline = BASE_MODEL.read_text()
    assert baseline.count(old) == 1
    model = baseline.replace(old, new)
    if case_name == "cast_audit_value_max_len_mask" and mask_declaration == "named":
        assert case["input"] == '17, audit_value_max_len=17 as usize, audit_mask_fields="name"'
        assert MODEL.read_text() == model
    return corpus, case, model


def adapted_rust_probe(source, rust_library, sql_fixture):
    text = source.read_text()
    changes = {
        '../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs':
            str((ROOT / 'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs').resolve()),
        '../../../support/dynamic_json_inherited_rows.sql':
            str(sql_fixture.resolve()),
        '../../../support/dynamic_json_privacy_values.sql':
            str((ROOT / 'tests/support/dynamic_json_privacy_values.sql').resolve()),
    }
    for old, new in changes.items():
        assert text.count(old) == 1
        text = text.replace(old, new)
    manifest = RUST_MANIFEST.read_text()
    assert manifest.count('../../../.reference/teaql-rs/') >= 2
    manifest = manifest.replace('../../../.reference/teaql-rs/', str(RUNTIME) + '/')
    old_library = '../../generator/parity/inheritance_graph_rust/lib'
    assert manifest.count(old_library) == 1
    manifest = manifest.replace(old_library, str(rust_library))
    assert manifest.count('[dependencies]') == 1
    manifest = manifest.replace('[dependencies]',
        '[dependencies]\nteaql-data-service = { path = "' + str(RUNTIME / 'teaql-data-service') + '" }')
    lock_text = RUST_LOCK.read_text()
    root_start = '[[package]]\nname = "teaql_dynamic_json_privacy_506"\n'
    assert lock_text.count(root_start) == 1
    before, root_and_after = lock_text.split(root_start, 1)
    old = ' "teaql-core",\n "teaql-runtime",\n]'
    new = ' "teaql-core",\n "teaql-data-service",\n "teaql-runtime",\n]'
    assert root_and_after.count(old) >= 1
    root_and_after = root_and_after.replace(old, new, 1)
    lock_text = before + root_start + root_and_after
    changes['direct_observer_dependency'] = 'teaql-data-service already present in source lock'
    return text, manifest, lock_text, changes


def adapted_cpp_fixture(source, sql_fixture):
    pattern = (r'std::string\(TEAQL_SOURCE_DIR\)\s*\+\s*'
               r'"/tests/support/dynamic_json_inherited_rows\.sql"')
    adapted, count = re.subn(pattern, lambda _: '"' + str(sql_fixture.resolve()) + '"', source)
    assert count == 1
    return adapted


def adapted_password_save(source, language):
    common = {
        'Update synthetic School name': 'Update synthetic School password',
        'SELECT name,version FROM school_data WHERE id=7':
            'SELECT password,version FROM school_data WHERE id=7',
    }
    native = {
        'field.name != "name"': 'field.name != "password"',
        'String("name", resource)': 'String("password", resource)',
        'school.updateName("PRIVATE-SCHOOL-CANARY")':
            'school.updatePassword("UPDATED-PASSWORD-CANARY")',
    }
    rust = {
        'field.name == "name"': 'field.name == "password"',
        '["name"]': '["password"]',
        'school.update_name("PRIVATE-SCHOOL-CANARY")':
            'school.update_password("UPDATED-PASSWORD-CANARY")',
    }
    for old, new in {**common, **(native if language == "cpp" else rust)}.items():
        assert source.count(old) == 1, (language, old, source.count(old))
        source = source.replace(old, new)
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--sink-record", type=Path, required=True)
    parser.add_argument("--source-case", choices=SOURCE_CASE_EXPECTATIONS,
                        default="cast_audit_value_max_len_mask")
    parser.add_argument("--mask-declaration", choices=("named", "empty", "absent",
                                                       "partial_unknown", "credential_field"),
                        default="named")
    parser.add_argument("--save-field", choices=("name", "password"), default="name")
    args = parser.parse_args()
    if args.save_field == "password" and args.mask_declaration != "credential_field":
        parser.error("Password Save requires the generated credential_field model")
    if any(path.exists() or path.suffix != ".gz" for path in (args.record, args.sink_record)):
        parser.error("Use fresh compressed receipt paths")
    corpus, source_case, model_source = checked_model(args.source_case, args.mask_declaration)
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    generator = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())[
        "generator_baseline_commit"]
    assert corpus["runtime_commit"] == runtime
    assert corpus["generator_commit"] == generator
    assert registry_identities(RUST_LOCK) <= registry_identities(RUNTIME / "Cargo.lock")
    java_sources = {
        label: sorted(path for package in ("model", "generator")
                      for path in (workspace / package / "src/main/java").rglob("*.java"))
        for label, workspace in (("original", ORIGINAL), ("cpp", PRODUCER))
    }
    libraries = [ROOT / "build_native_current" / package / ("lib" + package + ".a")
                 for package in ("teaql_provider_sqlite", "teaql_runtime", "teaql_sql",
                                 "teaql_data_service", "teaql_core", "teaql_memory")]
    inputs = [Path(__file__).resolve(), MODEL, BASE_MODEL, SOURCE_CASES, DEFAULT_MODEL_CASES,
              ROOT / "scripts/verify_generated_privacy_metadata.py",
              ROOT / "tests/parity/compare_values.py", RUST_PROBE, RUST_MANIFEST, RUST_LOCK,
              CPP_PROBE, RUST_METADATA_PROBE, CPP_METADATA_PROBE, RUST_SAVE_PROBE,
              CPP_SAVE_PROBE, SINK_VERIFIER, *libraries]
    inputs.extend(ROOT / "tests/support" / name for name in
                  ("dynamic_json_inherited_rows.sql", "dynamic_json_privacy_values.sql"))
    inputs.extend(java_sources["original"])
    inputs.extend(java_sources["cpp"])
    before = {str(path): digest(path) for path in inputs}
    record = {
        "scope": "Fresh original/C++ generated inheritance domain with a source-derived "
                 "audit expression; actual SQLite query, safe Context diagnostics and "
                 "ordinary/debug file-sink gates. Not arbitrary models or complete G14/G17/P7.",
        "complete": False,
        "runtime_commit": runtime,
        "generator_commit": generator,
        "model_sha256": hashlib.sha256(model_source.encode()).hexdigest(),
        "retained_model_sha256": digest(MODEL),
        "baseline_model_sha256": digest(BASE_MODEL),
        "source_case": source_case,
        "mask_declaration": args.mask_declaration,
        "save_field": args.save_field,
        "source_corpus_sha256": digest(SOURCE_CASES),
        "input_hashes_before": before,
        "free_bytes_before": shutil.disk_usage(ROOT).free,
        "steps": [],
    }

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + "\n").encode(), mtime=0))

    def run(label, command, *, cwd=ROOT, env=None, payload=None):
        print(label, "running", flush=True)
        result = subprocess.run(list(map(str, command)), cwd=cwd, env=env, input=payload,
                                text=True, capture_output=True, timeout=1200)
        record["steps"].append({"label": label, "command": list(map(str, command)),
                                "cwd": str(cwd), "exit_code": result.returncode,
                                "stdout": result.stdout, "stderr": result.stderr})
        save()
        print(label, "exit", result.returncode, flush=True)
        result.check_returncode()
        return result

    owned = None
    classpath_hashes = {}
    try:
        assert shutil.disk_usage(ROOT).free > 2 * 1024 ** 3
        run("upstream", [sys.executable, ROOT / "scripts/verify_upstream.py"])
        run("generator_ledger", [sys.executable, ROOT / "scripts/verify_generator_parity.py"])
        assert subprocess.check_output(["git", "-C", str(ORIGINAL), "rev-parse", "HEAD"],
                                       text=True).strip() == generator
        assert not subprocess.check_output(["git", "-C", str(ORIGINAL), "status",
                                            "--porcelain", "--untracked-files=no"]).strip()
        with tempfile.TemporaryDirectory(prefix="teaql_generated_audit_sql_506_") as name:
            owned = Path(name)
            record["owned_directory"] = name
            model = owned / "inheritance_graph.xml"
            model.write_text(model_source)
            sql_fixture = ROOT / "tests/support/dynamic_json_inherited_rows.sql"
            if args.mask_declaration == "credential_field":
                credential_fixture = owned / "credential_inherited_rows.sql"
                credential_fixture.write_text(sql_fixture.read_text() + "\n" + "\n".join(
                    "ALTER TABLE " + table + " ADD COLUMN password TEXT;\n"
                    "UPDATE " + table +
                    " SET password='PRIVATE-PASSWORD-CANARY' WHERE id=7;"
                    for table in ("school_data", "academy_data", "research_academy_data")) + "\n")
                sql_fixture = credential_fixture
                record["credential_fixture_sha256"] = digest(sql_fixture)
            classpaths = {}
            for label, workspace in (("original", ORIGINAL), ("cpp", PRODUCER)):
                classpath_file = owned / (label + "_classpath.txt")
                run(label + "_classpath", ["mvn", "-q", "-o", "-pl", "generator",
                    "dependency:build-classpath", f"-Dmdep.outputFile={classpath_file}"], cwd=workspace)
                dependencies = [Path(value) for value in classpath_file.read_text().strip().split(":")]
                classpath_hashes.update({str(path): digest(path) for path in dependencies})
                cp = ":".join(str(workspace / path) for path in
                              ("generator/target/classes", "model/target/classes"))
                cp += ":" + classpath_file.read_text().strip()
                classes = owned / (label + "_classes")
                classes.mkdir()
                extra = []
                if label == "original":
                    launcher = owned / "FrozenPrivacy.java"
                    launcher.write_text(LAUNCHER)
                    extra = [launcher]
                run(label + "_compile", ["javac", "--release", "17", "-cp", cp,
                    "-d", classes, *java_sources[label], *extra])
                classpaths[label] = ":".join(map(str, [classes,
                    workspace / "generator/src/main/resources",
                    workspace / "model/src/main/resources"])) + ":" + cp
                output = owned / (label + "_generated")
                main_class = ("FrozenPrivacy" if label == "original" else
                              "com.skynet.codegenerator.cpp.CppCoreGenerator")
                run(label + "_generate", ["java", "-cp", classpaths[label],
                    main_class, model, output])
                record[label + "_generated_hashes"] = file_hashes(output)
                save()

            original_generated = owned / "original_generated"
            cpp_generated = owned / "cpp_generated"
            metadata = json.loads((original_generated / "metadata.json").read_text())
            school = next(row for row in metadata if row["entity"] == "School")
            assert school["audit_limit"] == source_case["input"]
            assert school["mask_model_keys"] == (
                ["name"] if args.mask_declaration in ("named", "partial_unknown",
                                                     "credential_field") else [])
            record["school_model_metadata"] = school

            rust_project = owned / "rust_observer"
            (rust_project / "src").mkdir(parents=True)
            main_text, manifest, adapted_lock, adaptations = adapted_rust_probe(
                RUST_PROBE, original_generated / "lib", sql_fixture)
            (rust_project / "src/main.rs").write_text(main_text)
            (rust_project / "src/bin").mkdir()
            shutil.copyfile(RUST_METADATA_PROBE, rust_project / "src/bin/audit_metadata.rs")
            rust_save = RUST_SAVE_PROBE.read_text()
            fixture_path = "../../../support/dynamic_json_inherited_rows.sql"
            assert rust_save.count(fixture_path) == 1
            rust_save = rust_save.replace(
                fixture_path,
                str(sql_fixture.resolve()))
            if args.save_field == "password":
                rust_save = adapted_password_save(rust_save, "rust")
            (rust_project / "src/bin/audit_save.rs").write_text(rust_save)
            (rust_project / "Cargo.toml").write_text(manifest)
            (rust_project / "Cargo.lock").write_text(adapted_lock)
            record["rust_observer_adaptations"] = adaptations
            record["rust_observer_sha256"] = digest(rust_project / "src/main.rs")
            record["rust_save_observer_sha256"] = digest(rust_project / "src/bin/audit_save.rs")
            record["rust_lock_sha256"] = digest(rust_project / "Cargo.lock")
            cargo_target = owned / "target"
            rust_env = dict(os.environ, CARGO_TARGET_DIR=str(cargo_target),
                            CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER="/usr/bin/cc",
                            TEAQL_LOG_ENDPOINT="off")
            run("rust_build", ["cargo", "build", "--offline", "--locked",
                "--manifest-path", rust_project / "Cargo.toml", "--bins", "-j", "2"],
                env=rust_env)
            assert (rust_project / "Cargo.lock").read_text() == adapted_lock
            rust_binary = cargo_target / "debug/teaql_dynamic_json_privacy_506"
            rust_metadata_binary = cargo_target / "debug/audit_metadata"
            rust_save_binary = cargo_target / "debug/audit_save"

            original_cpp = CPP_PROBE.read_text()
            old_include = '#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"'
            assert original_cpp.count(old_include) == 1
            new_include = '#include "' + str(cpp_generated / "include/cpp_inheritance_graph.hpp") + '"'
            cpp_source = owned / "generated_privacy_probe.cpp"
            cpp_main = original_cpp.replace(old_include, new_include)
            if args.mask_declaration == "credential_field":
                cpp_main = adapted_cpp_fixture(cpp_main, sql_fixture)
            cpp_source.write_text(cpp_main)
            record["cpp_observer_sha256"] = digest(cpp_source)
            entries = json.loads((ROOT / "build_native_current/compile_commands.json").read_text())
            entry = next(row for row in entries if row["file"].endswith(
                "/tests/parity/generated_dynamic_json_privacy_probe.cpp"))
            command = list(entry.get("arguments") or shlex.split(entry["command"]))
            for flag in ("-MD", "-MT", "-MF", "-o"):
                while flag in command:
                    position = command.index(flag)
                    del command[position:position + (1 if flag == "-MD" else 2)]
            command.remove("-c")
            command.remove(entry["file"])
            cpp_binary = owned / "generated_privacy_probe"
            run("cpp_build", [*command, "-I" + str(ROOT / "tests/parity"), cpp_source,
                "-Wl,--start-group", *libraries, "-Wl,--end-group",
                ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a",
                "-pthread", "-ldl", "-lm", "-o", cpp_binary], cwd=ROOT / "build_native_current")
            cpp_metadata_binary = owned / "audit_metadata"
            run("cpp_metadata_build", [*command,
                "-I" + str(cpp_generated / "include"), CPP_METADATA_PROBE,
                "-Wl,--start-group", *libraries, "-Wl,--end-group",
                ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a",
                "-pthread", "-ldl", "-lm", "-o", cpp_metadata_binary],
                cwd=ROOT / "build_native_current")
            cpp_save_binary = owned / "audit_save"
            cpp_save_source = CPP_SAVE_PROBE
            if args.mask_declaration == "credential_field":
                cpp_save_source = owned / "audit_save.cpp"
                cpp_save_text = adapted_cpp_fixture(CPP_SAVE_PROBE.read_text(), sql_fixture)
                if args.save_field == "password":
                    cpp_save_text = adapted_password_save(cpp_save_text, "cpp")
                cpp_save_source.write_text(cpp_save_text)
            record["cpp_save_observer_sha256"] = digest(cpp_save_source)
            run("cpp_save_build", [*command,
                "-I" + str(cpp_generated / "include"), cpp_save_source,
                "-Wl,--start-group", *libraries, "-Wl,--end-group",
                ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a",
                "-pthread", "-ldl", "-lm", "-o", cpp_save_binary],
                cwd=ROOT / "build_native_current")

            source_metadata = json.loads(run("rust_metadata", [rust_metadata_binary],
                env=rust_env).stdout)
            current_metadata = json.loads(run("cpp_metadata", [cpp_metadata_binary],
                env=rust_env).stdout)
            record["original_metadata"] = source_metadata
            record["cpp_metadata"] = current_metadata
            record["metadata_match"] = same(source_metadata, current_metadata)
            assert record["metadata_match"] and len(source_metadata["descriptors"]) == 3
            school_metadata = source_metadata["descriptors"][0]
            assert school_metadata["entity"] == "School"
            name_masked = args.mask_declaration in ("named", "credential_field")
            expected_limit, expected_masked, expected_truncated = SOURCE_CASE_EXPECTATIONS[
                args.source_case]
            assert school_metadata["limit"] == (
                None if args.mask_declaration == "partial_unknown" else expected_limit)
            assert school_metadata["masks"] == (["name"] if name_masked else [])
            assert school_metadata["declared"] is (args.mask_declaration != "partial_unknown")
            expected_policy = {"named": "Masked", "empty": "Plain", "absent": "Plain",
                               "partial_unknown": "Unknown",
                               "credential_field": "Credential"}[args.mask_declaration]
            assert school_metadata["name_sql_policy"] == (
                "Masked" if args.mask_declaration == "credential_field" else expected_policy)
            assert school_metadata["password_sql_policy"] == (
                "Credential" if args.mask_declaration == "credential_field" else "Unknown")
            assert len(source_metadata["safe_audit"]) == 1
            safe_field = source_metadata["safe_audit"][0]
            assert safe_field["name"] == "name"
            if not name_masked:
                expected_masked, expected_truncated = "PRIVATE-SCHOOL-CANARY", False
            assert safe_field["masked"] is name_masked
            assert safe_field["truncated"] is expected_truncated
            assert safe_field["value"] == expected_masked
            assert safe_field["raw_length"] == 21
            assert safe_field["output_length"] == len(expected_masked)
            assert safe_field["mask_reason"] == (
                "_audit_mask_fields" if name_masked else None)
            assert safe_field["truncate_reason"] == (
                "_audit_value_max_len" if expected_truncated else None)
            assert ("PRIVATE-SCHOOL-CANARY" in json.dumps(source_metadata["safe_audit"])) is (
                not name_masked)
            expected_saved_field = dict(safe_field)
            if args.mask_declaration == "partial_unknown":
                expected_saved_field["value"] = "[REDACTED]"
                expected_saved_field["output_length"] = len("[REDACTED]")
            if args.save_field == "password":
                expected_saved_field.update({
                    "name": "password", "value": "[REDACTED]", "masked": True,
                    "truncated": False, "raw_length": len("UPDATED-PASSWORD-CANARY"),
                    "output_length": len("[REDACTED]"),
                    "mask_reason": "_audit_mask_fields",
                    "truncate_reason": None,
                })

            source_saves = {}
            current_saves = {}
            for mode in ("success", "write_failure", "sink_failure",
                         "policy_denied", "stale_version", "policy_approved",
                         "graph_success", "graph_policy_denied", "graph_policy_approved",
                         "graph_write_failure", "graph_stale_child", "graph_stale_root"):
                source_saves[mode] = json.loads(run(
                    "rust_save_" + mode, [rust_save_binary, mode], env=rust_env).stdout)
                current_saves[mode] = json.loads(run(
                    "cpp_save_" + mode, [cpp_save_binary, mode], env=rust_env).stdout)
            record["original_save"] = source_saves
            record["cpp_save"] = current_saves
            record["save_match"] = same(source_saves, current_saves)
            assert record["save_match"]
            for mode, expected in {
                "success": (True, "PRIVATE-SCHOOL-CANARY", 4, 1),
                "write_failure": (False, "school", 3, 0),
                "sink_failure": (False, "PRIVATE-SCHOOL-CANARY", 4, 1),
                "policy_denied": (False, "school", 3, 0),
                "stale_version": (False, "school", 4, 0),
                "policy_approved": (True, "PRIVATE-SCHOOL-CANARY", 4, 1),
                "graph_success": (True, "PRIVATE-SCHOOL-CANARY", 4, 2),
                "graph_policy_denied": (False, "school", 3, 0),
                "graph_policy_approved": (True, "PRIVATE-SCHOOL-CANARY", 4, 2),
                "graph_write_failure": (False, "school", 3, 0),
                "graph_stale_child": (False, "school", 3, 0),
                "graph_stale_root": (False, "school", 4, 0),
            }.items():
                result = source_saves[mode]
                assert result["mode"] == mode
                expected_db_name = expected[1]
                if args.save_field == "password":
                    expected_db_name = ("UPDATED-PASSWORD-CANARY" if expected[1] ==
                                        "PRIVATE-SCHOOL-CANARY" else "PRIVATE-PASSWORD-CANARY")
                assert (result["save_ok"], result["db_name"], result["version"],
                        result["event_count"]) == (
                            expected[0], expected_db_name, expected[2], expected[3])
                assert result["entity"] == ("School" if expected[3] else None)
                assert same(result["safe_field"], expected_saved_field if expected[3] else None)
                if expected[3]:
                    assert ("PRIVATE-SCHOOL-CANARY" in json.dumps(result["safe_field"])) is (
                        args.mask_declaration in ("empty", "absent"))
                    if args.save_field == "password":
                        assert "UPDATED-PASSWORD-CANARY" not in json.dumps(result["safe_field"])
                assert result["optimistic_conflict"] is (
                    mode in ("stale_version", "graph_stale_child", "graph_stale_root"))
                assert result["conflict_entity"] == (
                    "Enrollment" if mode == "graph_stale_child" else
                    "School" if mode in ("stale_version", "graph_stale_root") else None)
                assert result["policy_error"] is (mode in ("policy_denied", "graph_policy_denied"))
                assert result["fixture_write_failure"] is (
                    mode in ("write_failure", "graph_write_failure"))
                assert result["policy_reviews"] == (
                    ["School.saveGraph"] if mode in (
                        "policy_denied", "policy_approved", "graph_policy_denied",
                        "graph_policy_approved") else [])
                assert result["policy_operation_counts"] == (
                    [2] if mode in ("graph_policy_denied", "graph_policy_approved") else [1]
                    if mode in ("policy_denied", "policy_approved") else [])
                assert result["approved_event"] is (mode in ("policy_approved", "graph_policy_approved"))
                assert result["approved_event_entities"] == (
                    ["Enrollment", "School"] if mode == "graph_policy_approved" else
                    ["School"] if mode == "policy_approved" else [])
                assert result["event_entities"] == (
                    ["Enrollment", "School"] if mode in ("graph_success", "graph_policy_approved") else
                    ["School"] if expected[3] else [])
                assert (result["enrollment_note"], result["enrollment_version"]) == (
                    ("PRIVATE-NOTE-CANARY", 4)
                    if mode in ("graph_success", "graph_policy_approved") else
                    ("student", 4) if mode == "graph_stale_child" else ("student", 3))
            record["save_cases"] = len(source_saves)

            if args.save_field == "password":
                mutation_sinks = owned / "mutation_sinks"
                mutation_sinks.mkdir()
                observations = []
                for language, binary in (("rust", rust_save_binary),
                                         ("cpp", cpp_save_binary)):
                    for label, acknowledgment in (("unset", None), ("enabled", ACK)):
                        ordinary = mutation_sinks / (language + "_" + label + "_ordinary.log")
                        debug = mutation_sinks / (language + "_" + label + "_debug.log")
                        sink_env = {key: value for key, value in rust_env.items()
                                    if not key.startswith("TEAQL_")}
                        sink_env.update(TEAQL_LOG_ENDPOINT=str(ordinary),
                                        TEAQL_SQL_DEBUG_ENDPOINT=str(debug),
                                        TEAQL_SQL_LOG="_full", TEAQL_AUDIT_LOG="_full",
                                        TEAQL_LOG_FORMAT="human")
                        if acknowledgment:
                            sink_env["TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS"] = acknowledgment
                        result = json.loads(run("mutation_sink_" + language + "_" + label,
                                                [binary, "success"], env=sink_env).stdout)
                        assert same(result, source_saves["success"])
                        safe_text = ordinary.read_text() if ordinary.exists() else ""
                        debug_text = debug.read_text() if debug.exists() else ""
                        assert safe_text and bool(debug_text) is bool(acknowledgment)
                        expected_update = (
                            "UPDATE school_data SET password = '[REDACTED]' /* masked */, "
                            "version = 4 WHERE id = 7 AND version = 3")
                        assert expected_update in safe_text
                        if debug_text:
                            assert expected_update in debug_text
                        for text_value in (safe_text, debug_text):
                            assert "PRIVATE-PASSWORD-CANARY" not in text_value
                            assert "UPDATED-PASSWORD-CANARY" not in text_value
                            if text_value:
                                assert "[REDACTED]" in text_value
                        observations.append({"language": language, "case": label,
                                             "ordinary": safe_text, "debug": debug_text})
                record["mutation_sink_observations"] = observations
                record["mutation_sink_cases"] = len(observations)

            query_canary = ("PRIVATE-PASSWORD-CANARY" if args.mask_declaration ==
                            "credential_field" else "PRIVATE-SCHOOL-CANARY")
            fixture = {"entity": "school", "minimal": False, "mode": "list",
                       "shape": "plain", "logging": True,
                       "filters": [json.dumps({"password" if args.mask_declaration ==
                                               "credential_field" else "name":
                                               query_canary})]}
            payload = json.dumps(fixture) + "\n"
            source_result = json.loads(run("rust_query", [rust_binary],
                env=rust_env, payload=payload).stdout)
            current_result = json.loads(run("cpp_query", [cpp_binary],
                env=rust_env, payload=payload).stdout)
            record["query_input"] = fixture
            record["original_query"] = source_result
            record["cpp_query"] = current_result
            record["query_match"] = same(source_result, current_result)
            assert record["query_match"]
            assert same(source_result["rows"], [[7, 3, None]])
            assert source_result["error"] is None
            assert (query_canary in json.dumps(source_result["sql"])) is (
                expected_policy == "Plain")

            run("sink_matrix", [sys.executable, SINK_VERIFIER,
                "--cpp", cpp_binary, "--rust", rust_binary,
                "--output", args.sink_record,
                "--policy", expected_policy.lower(),
                "--field", "password" if args.mask_declaration == "credential_field" else "name",
                "--canary", query_canary],
                env=rust_env)
            record["sink_receipt"] = {"path": str(args.sink_record),
                                      "sha256": digest(args.sink_record)}
            with gzip.open(args.sink_record, "rt") as sink_file:
                sinks = json.load(sink_file)
            assert len(sinks["observations"]) == 12 and sinks["owned_directory_removed"]
            record["sink_cases"] = len(sinks["observations"])
            assert file_hashes(original_generated) == record["original_generated_hashes"]
            assert file_hashes(cpp_generated) == record["cpp_generated_hashes"]
            record["owned_bytes_removed"] = sum(
                path.stat().st_size for path in owned.rglob("*") if path.is_file())
    except Exception as error:
        record["controller_error"] = {"type": type(error).__name__, "message": str(error)}
        save()
        raise
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path): digest(path) for path in inputs}
        record["classpath_hashes"] = classpath_hashes
        record["classpath_unchanged"] = all(
            Path(name).is_file() and digest(Path(name)) == checksum
            for name, checksum in classpath_hashes.items())
        record["inputs_unchanged"] = record["input_hashes_after"] == before
        record["complete"] = (record.get("metadata_match", False) and
                              record.get("save_match", False) and record.get("save_cases") == 12 and
                              record.get("query_match", False) and record.get("sink_cases") == 12
                              and (record.get("mutation_sink_cases") == 4
                                   if args.save_field == "password" else True)
                              and not record.get("controller_error") and
                              record["owned_directory_removed"] and
                              record["inputs_unchanged"] and record["classpath_unchanged"])
        save()
    print("generated audit SQL/sinks:", record["sink_cases"], "cases passed", flush=True)
    return 0 if record["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
