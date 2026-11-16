#!/usr/bin/env python3
"""Qualify retained E values and diagnostics through complete compiler generation."""

import argparse
import base64
import copy
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
from types import SimpleNamespace

from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_generated_business_5_0_6 import (
    execution_inputs, manifest, native_inputs, read, references, require, run_owned, sha, write,
)
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_model_configuration_5_0_6 import worker_module
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_source_service_5_0_6 import artifact_sha
from compiler_scalar_source_admission_5_0_6 import frame_matches

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
PARITY = ROOT / "tests/parity"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
EVALUATOR = PRODUCER / "tools/cpp_evaluation/CppEvaluationProbe.java"
SOURCE_READER = PRODUCER / "tools/cpp_model_source/CppModelSource.java"
MODEL_NAMES = ("dynamic_json", "candidates", "inheritance")
ENTRIES = {"dynamic_json": "service", "candidates": "service", "inheritance": "controller"}
EVALUATION_ERRORS = {"dynamic_json": ["KSML-KEYWORD-001", "KSML-KEYWORD-002"],
                     "candidates": ["KSML-KEYWORD-001"], "inheritance": []}
CONSUMERS = {
    "values": ("generated_expression_values", 108, "87c0cd775233d7a6dc49790fd2cda1a60ad09f9fe087fd67deecab9a87ae2ff5"),
    "diagnostics": ("generated_expression_diagnostics", 196, "2c82e838644cf1da7059a34111a788ff588924bdb5613dae8dc2db44d0fee71b"),
    "text": ("expression_diagnostic_framing", 1200, "c328dd9517930f154e92e8f2d9c55e9be8d769f07961efe2a7161c446e419a5e"),
}
TEXT_PREFIX = PARITY / "expression_diagnostic_text_5_0_6_cases.json.gz"
TEXT_PREFIX_SHA = "d5d71d20ba2c36a11f791c82aa3db4dde965b5aacaf8cdd12a899e3efb3fb2d9"
sys.path.insert(0, str(PARITY))
from compare_values import json_lines, same
from compare_generated_expression_diagnostics_hardened_5_0_6 import adapt as adapt_diagnostic
from expression_diagnostic_framing import to_source


def models():
    return {name: (ROOT / "tests/generator" / (name + ".xml")).read_text() for name in MODEL_NAMES}


def sources(name):
    stem = "expression_diagnostic_text" if name == "text" else CONSUMERS[name][0]
    return PARITY / (stem + "_probe.cpp"), PARITY / "rust_field_layout/src/bin" / (stem + "_5_0_6.rs")


def corpus(name):
    stem, count, expected = CONSUMERS[name]
    path = PARITY / (stem + "_5_0_6_cases.json" + (".gz" if name == "text" else ""))
    require(sha(path.read_bytes()) == expected, "immutable complete E corpus: " + name)
    result = read(path) if name == "text" else json.loads(path.read_bytes())
    require(len(result["cases"]) == count, "complete E input membership")
    return result


def input_text(name):
    return "".join(json.dumps(row["input"], ensure_ascii=True) + "\n" for row in corpus(name)["cases"])


def adapted_source(name, owned):
    original = sources(name)[0].read_text()
    result, edits = original, []
    for model in MODEL_NAMES:
        namespace = "cpp_" + model
        before = '#include "../generated_' + model + '/include/' + namespace + '.hpp"'
        if before in result:
            require(result.count(before) == 1, "one authentic generated header per model")
            after = '#include "' + str(owned / model / "cpp/include" / (namespace + ".hpp")) + '"'
            result = result.replace(before, after, 1)
            edits.append(dict(before=before, after=after))
    require(len(edits) == {"values": 1, "diagnostics": 3, "text": 0}[name], "all generated includes accounted for")
    return result, edits


def consumer_manifest(owned, inventories):
    original = owned / "dynamic_json/original"
    base = manifest(original, "cpp_dynamic_json_core", sources("values")[1], inventories["dynamic_json"]["package"])
    header, tail = base.split("[[bin]]\n", 1)
    _, dependencies = tail.split("[dependencies]\n", 1)
    bins = "".join('[[bin]]\nname="e_' + name + '"\npath=' + json.dumps(str(sources(name)[1])) + "\n" for name in CONSUMERS)
    extra = "".join("cpp_" + name + "_core={package=" + json.dumps(inventories[name]["package"])
                    + ",path=" + json.dumps(str(owned / name / "original/lib")) + "}\n" for name in MODEL_NAMES[1:])
    return header + bins + "[dependencies]\n" + extra + dependencies


def expected_commands(record):
    owned, config = Path(record["owned_directory"]), record["capabilities"]
    command = config["cpp_generator"]["command"]
    cpp_classpath = command[command.index("-cp") + 1]
    result = {}
    for name in MODEL_NAMES:
        directory = owned / name
        call = [command[0], "-cp", cpp_classpath, "CppCompilerServiceProbe", str(directory / "model.json"),
                str(directory / "cpp"), str(Path(sys.executable).resolve()), str(WORKER),
                str(owned / "capabilities.json"), str(owned / "service_jobs"), str(directory / "job.json.gz")]
        if ENTRIES[name] == "service":
            result[name + "_controller_refusal"] = call + ["--controller"]
        result[name + "_" + ENTRIES[name]] = call + (["--controller"] if ENTRIES[name] == "controller" else [])
        result[name + "_original_generate"] = [*config["reference_generator"]["command"], str(directory / "model.json"),
            str(directory / "original"), str(directory / "inventory.json"), "--source-package", "--scalar-types"]
    path = str(owned / "consumer/Cargo.toml")
    result["original_consumer_resolve"] = ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", path]
    result["original_consumers_build"] = ["cargo", "build", "--offline", "--locked", "--bins", "-j", "2", "--manifest-path", path]
    for name in CONSUMERS:
        command = native_command(owned / "dynamic_json/cpp/include", owned / (name + ".cpp"), owned / ("native_" + name))
        command[1:1] = ["-I" + str(PARITY), *["-I" + str(owned / model / "cpp/include") for model in MODEL_NAMES[1:]]]
        result[name + "_native_build"] = list(map(str, command))
        result[name + "_original_run"] = [str(owned / "consumer_target/debug" / ("e_" + name))]
        result[name + "_native_run"] = [str(owned / ("native_" + name))]
    result["diagnostic_ownership"] = [str(owned / "native_text"), "--self-test"]
    return result


def compare(name, original, native):
    expected = [row["expected"] for row in corpus(name)["cases"]]
    require(len(original) == len(native) == len(expected) and same(original, expected), "complete authentic original E outcomes")
    if name == "values":
        require(same(native, original), "all actual E values/NULL/fallback outcomes")
    elif name == "diagnostics":
        require(same(native, [adapt_diagnostic(row) for row in original]), "complete structured E diagnostics with documented presentation only")
    else:
        require(all(to_source(value) == reference["raw"] for value, reference in zip(native, original, strict=True)), "complete actual raw framing with no repairs")
        require(sha(TEXT_PREFIX.read_bytes()) == TEXT_PREFIX_SHA, "immutable original text prefix")
        prefix = read(TEXT_PREFIX)
        require(len(prefix["cases"]) == 936 and prefix["cases"] == corpus(name)["cases"][:936]
                and same(original[:936], [row["expected"] for row in prefix["cases"]]), "all 936 retained raw text outcomes preserved")


def execute(path):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh E receipt and owned output space")
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  worker_source=WORKER.read_text(), inputs_before=execution_inputs(), references_before=references(),
                  native_inputs_before=native_inputs(), preparation_steps=[], evaluation_steps=[], steps=[], models=[], consumers=[])
    owned = None

    def run(label, command, *, env=None, input_text=None, expected=0, evaluation=False):
        command = list(map(str, command))
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=input_text, text=True, capture_output=True, timeout=900)
        record["evaluation_steps" if evaluation else "steps"].append(dict(label=label, command=command,
            input=input_text, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode == expected, label + " failed; inspect retained output")
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_expression_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capability_path, _ = prepare(owned, record)
                config = record["capabilities"]
                for name, model in models().items():
                    current = owned / name
                    current.mkdir()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(model.encode()).decode()})
                    (current / "model.json").write_text(json.dumps(package, sort_keys=True))
                    record["models"].append(dict(name=name, model=model, source_package=package, entry=ENTRIES[name]))
                inputs = [owned / name / "model.json" for name in MODEL_NAMES]
                for label, key in (("original", "reference_generator"), ("cpp", "cpp_generator")):
                    command = config[key]["command"]
                    classpath = command[command.index("-cp") + 1]
                    run(label + "_evaluation_javac", ["javac", "--release", "17", "-cp", classpath,
                        "-d", owned / ("original_classes" if label == "original" else "cpp_classes"),
                        EVALUATOR, SOURCE_READER], evaluation=True)
                    raw = run(label + "_evaluate", [command[0], "-cp", classpath, "CppEvaluationProbe", *inputs], evaluation=True)
                    values = [json.loads(line.removeprefix("TEAQL_EVALUATION_JSON=")) for line in raw.splitlines()
                              if line.startswith("TEAQL_EVALUATION_JSON=")]
                    require(len(values) == len(MODEL_NAMES), "complete independent evaluations")
                    for row, value in zip(record["models"], values, strict=True):
                        row[label + "_evaluation"] = value
                validate_evaluations(record)
                config["schema"] = "teaql.rust_configuration.job.v3"
                config["artifacts"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                capability_path.write_text(json.dumps(config, sort_keys=True))
                record["prepared_java_sources"].update({str(p): sha(p.read_bytes()) for p in (EVALUATOR, SOURCE_READER)})
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                    CARGO_TARGET_DIR=str(owned / "consumer_target"),
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"], TEAQL_LOG_ENDPOINT="off",
                    PYTHONDONTWRITEBYTECODE="1")
                commands = expected_commands(record)
                for row in record["models"]:
                    name = row["name"]
                    current = owned / name
                    source, cpp, report = current / "model.json", current / "cpp", current / "job.json.gz"
                    if row["entry"] == "service":
                        run(name + "_controller_refusal", commands[name + "_controller_refusal"], env=env, expected=1)
                        row.update(public_sdk_absent=not cpp.exists(), public_worker_report_absent=not report.exists())
                        require(row["public_sdk_absent"] and row["public_worker_report_absent"], "public refusal precedes worker dispatch")
                    run(name + "_" + row["entry"], commands[name + "_" + row["entry"]], env=env)
                    row.update(job=read(report), cpp_sources=hashes(cpp),
                        provenance=json.loads((cpp / "RUST_CONFIGURATION.json").read_bytes()),
                        service_jobs_remaining=list(map(str, (owned / "service_jobs").iterdir())),
                        compiler_jobs_remaining=list(map(str, (owned / "compiler_jobs").iterdir())))
                    raw = base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True)
                    require(json.loads(raw) == row["source_package"], "same source envelope for both generators")
                    source.write_bytes(raw)
                    run(name + "_original_generate", commands[name + "_original_generate"], env=env)
                    row.update(original_sources=hashes(current / "original"), original_inventory=json.loads((current / "inventory.json").read_bytes()))
                    require(row["original_sources"] == row["job"]["original_sources_before"], "same complete original library for compiler and consumer")
                inventories = {row["name"]: row["original_inventory"] for row in record["models"]}
                harness = owned / "consumer"
                harness.mkdir()
                record["consumer_manifest"] = consumer_manifest(owned, inventories)
                (harness / "Cargo.toml").write_text(record["consumer_manifest"])
                shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                run("original_consumer_resolve", commands["original_consumer_resolve"], env=env)
                require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "unchanged reference dependency identities")
                run("original_consumers_build", commands["original_consumers_build"], env=env)
                record["consumer_lock"] = (harness / "Cargo.lock").read_text()
                for name in CONSUMERS:
                    source, edits = adapted_source(name, owned)
                    (owned / (name + ".cpp")).write_text(source)
                    row = dict(name=name, cpp_source=sources(name)[0].read_text(), rust_source=sources(name)[1].read_text(),
                        adapted_source=source, include_edits=edits, corpus=corpus(name))
                    record["consumers"].append(row)
                    run(name + "_native_build", commands[name + "_native_build"], env=env)
                    data = input_text(name)
                    row["original"] = json_lines(run(name + "_original_run", commands[name + "_original_run"], env=env, input_text=data))
                    row["cpp"] = json_lines(run(name + "_native_run", commands[name + "_native_run"], env=env, input_text=data))
                    compare(name, row["original"], row["cpp"])
                run("diagnostic_ownership", commands["diagnostic_ownership"], env=env)
                for row in record["models"]:
                    row.update(original_sources_after=hashes(owned / row["name"] / "original"), cpp_sources_after=hashes(owned / row["name"] / "cpp"))
                record["artifacts_after"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                record["complete"] = True
            finally:
                for row in record["models"]:
                    report = owned / row["name"] / "job.json.gz"
                    if report.is_file() and "job" not in row:
                        row["job"] = read(report)
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(), inputs_after=execution_inputs(),
            references_after=references(), native_inputs_after=native_inputs())
        write(path, record)
    try:
        validate(record)
    except BaseException as error:
        record.update(complete=False, validation_error=repr(error))
        write(path, record)
        raise


def validate_evaluations(record):
    require([row["name"] for row in record["models"]] == list(MODEL_NAMES), "all authentic E models")
    for row in record["models"]:
        errors = [item["ruleId"] for item in row["original_evaluation"]["response"]["errors"]]
        require(row["model"] == models()[row["name"]] and row["entry"] == ENTRIES[row["name"]]
            and same(row["original_evaluation"], row["cpp_evaluation"])
            and errors == EVALUATION_ERRORS[row["name"]], "same public evaluation; reserved fixtures remain explicit")


def validate(record):
    from compiler_business_id_source_admission_5_0_6 import registered_epoch_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False
        and registered_epoch_matches(Path(__file__), sha(record["driver_source"].encode()))
        and registered_epoch_matches(WORKER, sha(record["worker_source"].encode())), "complete exact E execution")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
        and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
        and record["references_before"] == record["references_after"] == references()
        and record["native_inputs_before"] == record["native_inputs_after"] == native_inputs(), "stable source/reference/native frames and cleanup")
    require(record["capabilities"]["schema"] == "teaql.rust_configuration.job.v3"
        and [step["label"] for step in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
        and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["preparation_steps"]), "fresh independent Java preparation and explicit scalar capability")
    require([step["label"] for step in record["evaluation_steps"]] == ["original_evaluation_javac", "original_evaluate", "cpp_evaluation_javac", "cpp_evaluate"]
        and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["evaluation_steps"])
        and all(registered_epoch_matches(Path(name), value) for name, value in record["prepared_java_sources"].items()), "complete actual evaluation and Java source binding")
    validate_evaluations(record)
    steps = record["evaluation_steps"]
    for index, label in ((1, "original"), (3, "cpp")):
        values = [json.loads(line.removeprefix("TEAQL_EVALUATION_JSON=")) for line in steps[index]["stdout"].splitlines() if line.startswith("TEAQL_EVALUATION_JSON=")]
        require(same(values, [row[label + "_evaluation"] for row in record["models"]]), "actual independent evaluation stdout")
    worker = worker_module()
    for row in record["models"]:
        job = row["job"]
        require(row["source_package"] == dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
            files={"main.xml": base64.b64encode(row["model"].encode()).decode()}), "every original model byte retained")
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(json.loads(raw) == row["source_package"] and sha(raw) == job["model_sha256"]
            and job["worker_sha256"] == sha(record["worker_source"].encode())
            and job["capabilities_sha256"] == sha(json.dumps(record["capabilities"], sort_keys=True).encode()), "same model/worker/capability binding")
        require(job["complete"] is True and job["outcome"] == "resolved" and job["owned_directory_removed"] is True
            and job["publication_directory_removed"] is True and row["service_jobs_remaining"] == row["compiler_jobs_remaining"] == [], "complete compiler jobs and cleanup")
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        worker.validate_scalar_types(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
        document = job["resolution_document"]
        require(document["schema"] == "teaql.rust_configuration.model_generation.v2"
            and document["scalar_types"] == job["scalar_types"]
            and document["scalar_observer_sha256"] == sha(job["scalar_observer_source"].encode())
            and job["scalar_observer_source"] == worker.scalar_source(job["original_inventory"]["entities"])
            and document["scalar_artifacts_sha256"] == worker.manifest_sha(job["scalar_artifacts"])
            and job["scalar_artifacts"] == job["configuration_artifacts"] == job["observer_artifacts"], "same-library scalar/descriptor/MIR provenance")
        require(row["original_inventory"] == job["original_inventory"]
            and row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"]
            and row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"]
            and row["provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode()), "all complete original/native SDKs immutable")
        if row["entry"] == "service":
            require(row["public_sdk_absent"] is True and row["public_worker_report_absent"] is True, "public reserved fixture remains refused")
    owned = Path(record["owned_directory"])
    inventories = {row["name"]: row["original_inventory"] for row in record["models"]}
    require(record["consumer_manifest"] == consumer_manifest(owned, inventories)
        and identities(SimpleNamespace(read_text=lambda: record["consumer_lock"])) <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "full library/source/dependency binding")
    require([row["name"] for row in record["consumers"]] == list(CONSUMERS), "complete E consumer inventory")
    actual = {step["label"]: step for step in record["steps"]}
    expected = expected_commands(record)
    require(len(actual) == len(record["steps"]) == len(expected)
        and {name: step["command"] for name, step in actual.items()} == expected
        and all(type(step["exit_code"]) is int and step["exit_code"] == (1 if name.endswith("_controller_refusal") else 0)
                for name, step in actual.items()), "every exact entry/compiler/consumer command and terminal outcome")
    for row in record["consumers"]:
        name = row["name"]
        source, edits = adapted_source(name, owned)
        require(row["cpp_source"] == sources(name)[0].read_text() and row["rust_source"] == sources(name)[1].read_text()
            and row["adapted_source"] == source and row["include_edits"] == edits and row["corpus"] == corpus(name), "unchanged consumers and complete corpora; generated includes only")
        require(actual[name + "_original_run"]["input"] == actual[name + "_native_run"]["input"] == input_text(name)
            and row["original"] == json_lines(actual[name + "_original_run"]["stdout"])
            and row["cpp"] == json_lines(actual[name + "_native_run"]["stdout"]), "actual full input/stdout binding")
        compare(name, row["original"], row["cpp"])
    require(actual["diagnostic_ownership"]["stdout"] == "" and record["artifacts_after"] == record["capabilities"]["artifacts"], "ownership self-test and immutable Java artifacts")


def controls(record):
    validate(record)
    changes = (
        ("unfinished", lambda r: r.update(complete=False)),
        ("missing_model", lambda r: r["models"].pop()),
        ("changed_model", lambda r: r["models"][0].update(model="<root/>")),
        ("changed_evaluation", lambda r: r["models"][1]["cpp_evaluation"]["response"].update(errors=[])),
        ("wrong_entry", lambda r: r["models"][1].update(entry="controller")),
        ("old_capability", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v2")),
        ("missing_scalar", lambda r: r["models"][0]["job"]["scalar_types"][0]["properties"].pop()),
        ("wrong_library", lambda r: r["models"][0]["job"]["scalar_artifacts"].update(invented="0" * 64)),
        ("changed_source", lambda r: r["models"][0]["original_sources"].update(invented="0" * 64)),
        ("changed_cpp_body", lambda r: r["consumers"][0].update(adapted_source="int main(){}\n")),
        ("changed_rust_driver", lambda r: r["consumers"][0].update(rust_source="fn main(){}\n")),
        ("missing_consumer", lambda r: r["consumers"].pop()),
        ("missing_case", lambda r: r["consumers"][0]["corpus"]["cases"].pop()),
        ("missing_actual_row", lambda r: r["consumers"][1]["cpp"].pop()),
        ("wrong_raw_frame", lambda r: r["consumers"][2]["original"][0].update(raw="repaired")),
        ("wrong_cpp_diagnostic", lambda r: r["consumers"][1]["cpp"][0].update(severity="warning")),
        ("wrong_input", lambda r: next(s for s in r["steps"] if s["label"] == "values_native_run").update(input="{}\n")),
        ("changed_stdout", lambda r: next(s for s in r["steps"] if s["label"] == "values_native_run").update(stdout="{}\n")),
        ("changed_command", lambda r: next(s for s in r["steps"] if s["label"] == "dynamic_json_service")["command"].append("--controller")),
        ("wrong_manifest", lambda r: r.update(consumer_manifest="[package]\nname=\"wrong\"\n")),
        ("hidden_public_refusal", lambda r: next(s for s in r["steps"] if s["label"] == "candidates_controller_refusal").update(exit_code=0)),
        ("missing_process", lambda r: r["steps"].pop()),
        ("boolean_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("missing_cleanup", lambda r: r.update(owned_directory_removed=False)),
    )
    result = []
    for name, mutate in changes:
        bad = copy.deepcopy(record)
        mutate(bad)
        try:
            validate(bad)
        except (RuntimeError, KeyError, AssertionError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler E corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", required=True, type=Path)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.record.resolve())
    else:
        result = controls(read(args.record))
        if args.controls:
            require(not args.controls.exists(), "preserve earlier E controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                execution_sha256=sha(args.record.read_bytes()), controls=result))
        print("Verified complete compiler E consumers:", len(result), "controls; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
