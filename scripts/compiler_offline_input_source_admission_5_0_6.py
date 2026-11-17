#!/usr/bin/env python3
"""Qualify complete offline XML/ZIP inputs without relabeling prior executions."""

import argparse
import ast
import base64
import copy
from functools import cache
import io
import json
from pathlib import Path
import re
import sys
import zipfile

from verify_compiler_generated_business_5_0_6 import read, require, sha, write

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
CORE = "producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"
TEST = "producer:generator/src/test/java/com/skynet/codegenerator/cpp/CppOfflineModelInputTest.java"
PREVIOUS = "runtime:scripts/compiler_offline_source_admission_5_0_6.py"
SELF = "runtime:scripts/compiler_offline_input_source_admission_5_0_6.py"
RECORDS = {
    "initial": ("compiler_offline_inputs_initial_5_0_6.json.gz", "c7c50a5df80194893df0b567b15433daf2ed010a13f9308be596052ea2eb4d02"),
    "units": ("compiler_offline_inputs_units_5_0_6.json.gz", "1516444b1a38ea5806cd85a966cdf3ceb6698223913cabc0355a4e045451f675"),
    "sdks": ("compiler_offline_inputs_sdks_5_0_6.json.gz", "045e1c41dc6d8bf87fdda4fd177958ad3e3a2f2dee069c2b4d43f711ad5e5cdf"),
    "defaults": ("compiler_offline_inputs_default_sdks_5_0_6.json.gz", "836cd7973ef491fd82470e029c2d5e9d74568fc3789e678db7438d30e208ae78"),
    "sources": ("compiler_offline_inputs_sources_5_0_6.json.gz", "36fe4a7bee0a19c2991a05c9ef25f6cb6a87c130c8ce8f6ff662d32b14c7de6a"),
}


@cache
def bound(name):
    filename, expected = RECORDS[name]
    path = EVIDENCE / filename
    require(sha(path.read_bytes()) == expected, "immutable offline input receipt: " + filename)
    return read(path)


def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("producer", "runtime") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical offline input source identity")
    return (PRODUCER if scope == "producer" else ROOT) / relative


def validate_sources(record):
    from narrow_integer_registration_source_admission_5_0_6 import current_source_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {CORE, TEST, PREVIOUS}, "exact input-loader, CLI contracts and identity guards")
    initial, units = bound("initial"), bound("units")
    for name, row in record["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and current_source_matches(source_path(name), row["after_sha256"]),
                "exact current input source image")
        if row["before_source"] is None:
            require(row["before_sha256"] is None and name not in initial["inputs_after"], "genuinely added source")
        else:
            require(row["before_sha256"] == sha(row["before_source"].encode()) == initial["inputs_after"][name],
                    "exact executed preceding source image")
        if name.startswith("producer:"):
            require(row == units["source_images"][name.split(":", 1)[1]], "actual freshly compiled Java input")
    row = record["paths"][CORE]
    old = ('    GenerateContext context = new GenerateContext(SCOPE, model.getFileName().toString());\n'
           '    context.setEntranceFile(model.getFileName().toString());\n'
           '    byte[] modelBytes = Files.readAllBytes(model);\n'
           '    context.appendFile(model.getFileName().toString(), modelBytes);\n')
    new = ('    byte[] modelBytes = Files.readAllBytes(model);\n'
           '    GenerateContext context = new ModelInputLoader().load(modelBytes, model.getFileName().toString(), SCOPE);\n'
           '    if (context.getEntranceFile() == null) {\n'
           '      throw new IllegalArgumentException("Missing model entrance file: main.xml");\n'
           '    }\n')
    require(row["after_source"].count(new) == 1 and row["after_source"].replace(new, old, 1).replace(
        "import com.skynet.codegenerator.input.ModelInputLoader;\n", "", 1) == row["before_source"],
        "only main input loading changes; all emitters remain byte-identical")
    row = record["paths"][PREVIOUS]
    trees = [ast.parse(row[name]) for name in ("before_source", "after_source")]
    changed = ["validate_sources", "manifest_rows", "source_epochs", "added_input_hashes"]
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)} for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and [name for name in functions[0] if functions[0][name] != functions[1][name]] == changed,
            "four exact identity-guard continuations")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in changed:
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "all old execution/comparison/control and module bodies remain exact")


@cache
def sources():
    record = bound("sources")
    validate_sources(record)
    return record["paths"]


@cache
def manifest_rows():
    record = bound("sdks")
    require(record["complete"] is True and len(record["before"]) == len(record["after"]) == 80,
            "all eighty regenerated input-compatible SDKs")
    result = {}
    for before, after in zip(record["before"], record["after"], strict=True):
        name, leaf = before["manifest"], Path(before["manifest"]).name
        require(name == after["manifest"] and before["files"].keys() == after["files"].keys()
                and all(after["files"][key] == value for key, value in before["files"].items() if key != leaf),
                "every generated code and Assist byte remains unchanged")
        expected = copy.deepcopy(before["value"])
        key = CORE.split(":", 1)[1]
        require(expected["generator_implementation_sha256"][key] == sources()[CORE]["before_sha256"],
                "actual preceding input entry identity")
        expected["generator_implementation_sha256"][key] = sources()[CORE]["after_sha256"]
        require(expected == after["value"] and sha(before["text"].encode()) == before["files"][leaf]
                and sha(after["text"].encode()) == after["files"][leaf] == sha((ROOT / name).read_bytes()),
                "only the proven input-entry fingerprint changes")
        result["runtime:" + name] = dict(before_source=before["text"], before_sha256=before["files"][leaf],
                                        after_source=after["text"], after_sha256=after["files"][leaf])
    return result


def qualified_transition(name, before, after):
    row = (sources() if name in (CORE, TEST, PREVIOUS) else manifest_rows()).get(name)
    return row is not None and row["before_sha256"] == before and row["after_sha256"] == after


def source_rows():
    from compiler_codec_module_source_admission_5_0_6 import source_rows as module_rows
    return {**sources(), **manifest_rows(), **module_rows()}


def added_input_hashes():
    require(SELF not in bound("initial")["inputs_after"], "new input validator")
    from compiler_codec_module_source_admission_5_0_6 import added_input_hashes as module_added
    return {TEST: sources()[TEST]["after_sha256"], SELF: sha(Path(__file__).read_bytes()), **module_added()}


def validate_units(record):
    from compiler_offline_source_admission_5_0_6 import frame_matches, registered_epoch_matches
    from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, references
    require(record == bound("units") and record["complete"] is True and record["whole_goal_complete"] is False
            and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and record["native_before"] == record["native_after"] == native_inputs()
            and record["references_before"] == record["references_after"] == references(), "actual complete frozen input contracts")
    require([(row["label"], row["exit_code"]) for row in record["steps"]] == [
        ("current_tests_javac", 0), ("current_offline_contracts", 0), ("current_all_contracts", 0),
        ("previous_main_javac", 0), ("previous_offline_contracts", 1)]
        and all(type(row["exit_code"]) is int for row in record["steps"] + record["preparation_steps"]),
        "actual five terminal current/previous Java commands")
    require("OK (5 tests)" in record["steps"][1]["stdout"] and "OK (287 tests)" in record["steps"][2]["stdout"]
            and "Tests run: 5,  Failures: 4" in record["steps"][-1]["stdout"], "new/all/preceding-main actual regression counts")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and all(registered_epoch_matches(Path(name), digest) for name, digest in record["prepared_java_sources"].items()),
            "compiled current Java identities and cleanup")


def validate_defaults(record):
    from compiler_offline_source_admission_5_0_6 import frame_matches
    from preserve_compiler_projection_contracts_5_0_6 import BATCH, cases
    from verify_compiler_generated_business_5_0_6 import execution_inputs, references
    require(record == bound("defaults") and record["complete"] is True and record["whole_goal_complete"] is False
            and type(record["actual_cpp_compiler_processes"]) is int and record["actual_cpp_compiler_processes"] == 0
            and record["reused_cpp_compiler_outcomes"] == 406 and record["planned_cases"] == cases()
            and len(record["cases"]) == 406 and record["batch_source"] == BATCH, "actual 406 defaults with explicit compiler-outcome reuse")
    require(all(actual["id"] == expected["id"] and actual["match"] is True and actual["files"] == expected["files"]
                for actual, expected in zip(record["cases"], record["planned_cases"], strict=True)), "every complete default SDK matches")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and frame_matches(execution_inputs(), {key: value for key, value in record["input_hashes_after"].items()
                                                  if key != "runtime:docs/cpp_generator_parity_ledger.json"})
            and record["references_before"] == record["references_after"] == references()
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "frozen default sources and cleanup")
    require([row["label"] for row in record["steps"]] == ["current_javac", "complete_sdk_batch"]
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0 for row in record["steps"])
            and "CPP_SDK_COUNT 406" in record["steps"][-1]["stdout"], "actual default batch commands and raw result")


def validate_sdks(record):
    import reproduce_offline_compiler_sdks_5_0_6 as adapter
    import reproduce_evaluated_generator_sdks_5_0_6 as engine
    require(record == bound("sdks"), "immutable actual input-compatible SDK reproduction")
    saved = adapter.BASE, adapter.BASE_SHA, adapter.BEFORE_SHA
    try:
        adapter.BASE = EVIDENCE / "compiler_offline_route_sdks_5_0_6.json.gz"
        adapter.BASE_SHA = "805942cafa3f2c5d26c0536c223c3ca1c0cd1169b06b24db787fda02dbe3af16"
        adapter.BEFORE_SHA = sources()[CORE]["before_sha256"]
        adapter.configure()
        engine.validate(record)
    finally:
        adapter.BASE, adapter.BASE_SHA, adapter.BEFORE_SHA = saved
        adapter.configure()


def model_files(flat):
    school = re.findall(r'    <school\b[^<>]*/>\n', flat)
    enrollment = '    <enrollment school="school()" note="string()" />\n'
    require(len(school) == 1 and flat.count(enrollment) == 1, "exact governed fixture decomposition")
    entrance = flat.replace(school[0], '    <_include file="entities/school.xml"/>\n', 1).replace(
        enrollment, '    <_include file="../common/enrollment.xml"/>\n', 1)
    return {"models/main.xml": entrance.encode(),
            "models/entities/school.xml": ("<root>\n" + school[0] + "</root>\n").encode(),
            "common/enrollment.xml": ("<root>\n" + enrollment + "</root>\n").encode(),
            "data/binary.bin": bytes((0, 255, 7, 3))}


def validate_business(record, sinks):
    import verify_primitive_const_governed_execution_5_0_6 as engine
    from compiler_offline_source_admission_5_0_6 import frame_matches
    from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, references
    engine.validate(record, sinks)
    package = record["archive_source_package"]
    files = model_files(record["model_source"])
    require(package == dict(schema="teaql.model_source_package.v1", entrance_file="models/main.xml",
                            files={name: base64.b64encode(data).decode() for name, data in files.items()})
            and record["archive_file_sha256"] == {name: sha(data) for name, data in files.items()},
            "unchanged modeled declarations and exact derived source files")
    raw = base64.b64decode(record["archive_bytes_base64"], validate=True)
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        require(set(archive.namelist()) == set(package["files"]) and len(archive.namelist()) == 4
                and package["entrance_file"] == "models/main.xml"
                and all(archive.read(name) == base64.b64decode(value, validate=True)
                        for name, value in package["files"].items()), "actual complete archive source membership and bytes")
    require(json.loads(base64.b64decode(record["archive_package_bytes_base64"], validate=True)) == package
            and record["archive_flat_model_source"] == record["model_source"], "same modeled source in both independent routes")
    original = {name: digest for name, digest in record["archive_original_sources"].items() if name != "metadata.json"}
    digest = sha("".join(name + "\0" + original[name] + "\n" for name in sorted(original, key=lambda name: name.encode())).encode())
    require(digest == record["archive_original_manifest_sha256"] == record["offline_cpp_provenance"]["source_manifest_sha256"]
            and record["archive_original_sources"] == record["original_generated_hashes"], "compiler and independent original SDK identity")
    from verify_generated_privacy_metadata import LAUNCHER
    bridge = record["archive_reference_bridge"]
    old = ('        GenerateContext context = new GenerateContext("rust-lib-core", input.getFileName().toString());\n'
           '        context.setEntranceFile(input.getFileName().toString());\n'
           '        context.appendFile(input.getFileName().toString(), Files.readAllBytes(input));')
    require(bridge["before_source"] == LAUNCHER and bridge["before_sha256"] == sha(LAUNCHER.encode())
            and LAUNCHER.count(old) == 1 and bridge["after_source"] == LAUNCHER.replace(
                old, '        GenerateContext context = CppModelSource.read(input, "rust-lib-core", true);', 1)
            and bridge["after_sha256"] == sha(bridge["after_source"].encode()),
            "independent original bridge changes only input loading")
    process, refusal = record["offline_process"], record["offline_reference_refusal"]
    require(type(process["exit_code"]) is int and process["exit_code"] == 0 and process["command"][-2].endswith("complete_model.zip")
            and all("-D" + name + "=" + value in process["command"] for name, value in record["offline_properties"].items()),
            "actual trusted default ZIP CLI execution")
    require(record["offline_cpp_provenance"]["schema"] == "teaql.rust_configuration.model_generation.v3"
            and record["capabilities"]["schema"] == "teaql.rust_configuration.job.v4"
            and record["offline_cpp_sources"] == record["cpp_generated_hashes"]
            and record["offline_artifacts_after"] == record["capabilities"]["artifacts"], "complete actual SDK and semantic compiler facts")
    require(type(refusal["exit_code"]) is int and refusal["exit_code"] != 0 and refusal["sdk_exists"] is False
            and "REFERENCE_COMPILER_REFUSAL" in refusal["stderr"] and "E0277" in refusal["stderr"]
            and refusal["command"][-2].endswith("refused.zip"), "actual ZIP compiler refusal without fallback SDK")
    with zipfile.ZipFile(io.BytesIO(base64.b64decode(record["archive_refusal_bytes_base64"], validate=True))) as archive:
        require(set(archive.namelist()) == {"models/main.xml", "data/binary.bin"}
                and archive.read("models/main.xml") == refusal["model_source"].encode(), "actual refusal archive model")
    commands = [row for row in record["process_adaptations"]
                if "com.skynet.codegenerator.cpp.CppCoreGenerator" in row["source_command"]]
    require(len(commands) == 1 and commands[0]["executed_command"] == process["command"], "one actual default CLI dispatch")
    require(record["offline_adapter_sha256"] == sha(record["offline_adapter_source"].encode())
            and record["offline_inputs_before"] == record["offline_inputs_after"]
            and frame_matches(execution_inputs(), record["offline_inputs_after"])
            and record["offline_native_before"] == record["offline_native_after"] == native_inputs()
            and record["offline_references_before"] == record["offline_references_after"] == references(), "frozen actual business source/native/reference inputs")
    require(record["offline_directory_removed"] is True and not Path(record["offline_directory"]).exists()
            and record["offline_service_jobs_remaining"] == record["offline_compiler_jobs_remaining"] == [], "all owned compiler jobs reclaimed")


def evidence():
    sources()
    manifest_rows()
    validate_sdks(bound("sdks"))
    validate_units(bound("units"))
    validate_defaults(bound("defaults"))
    validate_business(read(EVIDENCE / "compiler_offline_inputs_complete_5_0_6.json.gz"),
                      read(EVIDENCE / "compiler_offline_inputs_complete_sinks_5_0_6.json.gz"))


def controls():
    evidence()
    business = read(EVIDENCE / "compiler_offline_inputs_complete_5_0_6.json.gz")
    sinks = read(EVIDENCE / "compiler_offline_inputs_complete_sinks_5_0_6.json.gz")
    mutations = (
        ("sources", "missing_input_source", lambda row: row["paths"].pop(CORE)),
        ("sources", "unqualified_main", lambda row: row["paths"][CORE].update(after_sha256="0" * 64)),
        ("sources", "forged_previous_guard", lambda row: row["paths"][PREVIOUS].update(before_source="pass\n")),
        ("units", "unfinished_java", lambda row: row.update(complete=False)),
        ("units", "boolean_java_exit", lambda row: row["steps"][0].update(exit_code=False)),
        ("units", "lost_previous_regression", lambda row: row["steps"][-1].update(exit_code=0)),
        ("defaults", "missing_default_sdk", lambda row: row["cases"].pop()),
        ("defaults", "invented_compilation", lambda row: row.update(actual_cpp_compiler_processes=406)),
        ("business", "lost_archive_source", lambda row: row["archive_source_package"]["files"].pop("data/binary.bin")),
        ("business", "wrong_entrance", lambda row: row["archive_source_package"].update(entrance_file="missing.xml")),
        ("business", "wrong_original_sdk", lambda row: row["archive_original_sources"].pop(next(iter(row["archive_original_sources"])))),
        ("business", "boolean_cli_exit", lambda row: row["offline_process"].update(exit_code=False)),
        ("business", "older_metadata", lambda row: row["offline_cpp_provenance"].update(schema="teaql.rust_configuration.model_generation.v1")),
        ("business", "false_graph_success", lambda row: row["cpp_save"]["graph_write_failure"].update(save_ok=True)),
        ("business", "fallback_sdk", lambda row: row["offline_reference_refusal"].update(sdk_exists=True)),
        ("business", "lost_job_cleanup", lambda row: row.update(offline_service_jobs_remaining=["pending"])),
    )
    result = []
    for kind, name, mutate in mutations:
        row = copy.deepcopy(business if kind == "business" else bound(kind))
        mutate(row)
        try:
            if kind == "business":
                validate_business(row, sinks)
            else:
                {"sources": validate_sources, "units": validate_units, "defaults": validate_defaults}[kind](row)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted offline input corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve input qualification controls")
        write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls()))
    print("Verified complete offline XML/ZIP inputs; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
