#!/usr/bin/env python3
"""Run retained JSON input/graph consumers through complete compiler generation."""

import argparse
import base64
import copy
import json
import os
from pathlib import Path
import re
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

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
PARITY = ROOT / "tests/parity"
sys.path.insert(0, str(PARITY))
from compare_values import json_lines, same

MODEL_NAMES = ("typed_member_aliases", "sample_fields_runtime")
MODEL_ENTRIES = {"typed_member_aliases": "service", "sample_fields_runtime": "controller"}
CONSUMERS = {
    "inputs": ("generated_json_inputs", "generated_json_boundaries", 5),
    "nullable": ("generated_json_nullable_graph", "generated_json_nullable_graph", 20),
    "owned": ("generated_json_owned_nullable", "generated_json_owned_nullable", 1),
    "ambiguous": ("generated_json_ambiguous", "generated_json_ambiguous", 2),
}
CORPUS_HASHES = {
    "inputs": "80725ea5b7e83dad2597fc07e252d3f08d24d748423f139bd92849747acd141e",
    "nullable": "d3c20c386882334780b744ac57eea3315bef22b0ca8c1de61d850e5634a79d44",
    "owned": "0dcad079022478d1ec4fe610a4d4e2096b6a45dd719422306a35d76d1e5feec6",
    "ambiguous": "0d3e59e574f84b429bdcb3585d801e7a4e5db24902683335c2848e44e1d5506d",
}
DIAGNOSTIC = ROOT / "docs/parity/generated_json_inputs_diagnostic_comparison_5_0_6.json.gz"
DIAGNOSTIC_SHA = "395d8f9578374ffc238b19530cada499fc2cd45fbdb041965fa4dfc2dde7dac4"
EVALUATION = ROOT / "docs/parity/compiler_json_consumers_evaluation_diagnosis_5_0_6.json.gz"
EVALUATION_SHA = "39f4ebfbf9431c894f9734804e0c23844a1e4945949e8d7e479ed921e59ae269"


def models():
    return {name: (ROOT / "tests/generator" / (name + ".xml")).read_text() for name in MODEL_NAMES}


def evaluation():
    require(sha(EVALUATION.read_bytes()) == EVALUATION_SHA, "immutable independent model evaluations")
    record = read(EVALUATION)
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"] == references()
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "complete independent evaluation and cleanup")
    require([row["name"] for row in record["cases"]] == list(MODEL_NAMES)
            and all(row["model"] == models()[row["name"]] and same(row["original"], row["cpp"])
                    for row in record["cases"]), "same unchanged models and exact original/current evaluation")
    require([item["ruleId"] for item in record["cases"][0]["original"]["response"]["errors"]]
            == ["KSML-KEYWORD-001", "KSML-KEYWORD-002", "KSML-KEYWORD-002"]
            and record["cases"][1]["original"]["response"]["errors"] == [], "public reserved-name refusal and valid scalar model")
    return record


def consumer_sources(name):
    stem = CONSUMERS[name][0]
    return PARITY / (stem + "_probe.cpp"), PARITY / "rust_field_layout/src/bin" / (stem + "_5_0_6.rs")


def corpus(name):
    path = PARITY / (CONSUMERS[name][1] + "_5_0_6_cases.json.gz")
    require(sha(path.read_bytes()) == CORPUS_HASHES[name], "immutable original JSON corpus")
    record = read(path)
    require(len(record["cases"]) == CONSUMERS[name][2]
            and record["baseline"] == references()["teaql-rs"]
            and record["driver_sha256"] == sha(consumer_sources(name)[1].read_bytes()), "complete original JSON consumer inputs")
    return record


def adapted_cpp(name, sdk_paths):
    source = consumer_sources(name)[0].read_text()
    edits = []
    for model in MODEL_NAMES:
        namespace = "cpp_member_aliases" if model == "typed_member_aliases" else "cpp_sample_runtime"
        before = '#include "../generated_' + model + '/include/' + namespace + '.hpp"'
        if before not in source:
            continue
        after = '#include "' + str(sdk_paths[model] / "include" / (namespace + ".hpp")) + '"'
        require(source.count(before) == 1, "one authentic generated include per model")
        source = source.replace(before, after, 1)
        edits.append(dict(before=before, after=after, model=model))
    require(len(edits) == (2 if name == "inputs" else 1), "all retained generated includes redirected")
    return source, edits


def cargo_manifest(originals, packages):
    first = originals["typed_member_aliases"]
    base = manifest(first, "cpp_member_aliases_core", consumer_sources("inputs")[1], packages["typed_member_aliases"])
    header, body = base.split("[[bin]]\n", 1)
    _, dependencies = body.split("[dependencies]\n", 1)
    sample = originals["sample_fields_runtime"]
    package = packages["sample_fields_runtime"]
    bins = "".join('[[bin]]\nname="json_' + name + '"\npath=' + json.dumps(str(consumer_sources(name)[1])) + "\n"
                   for name in CONSUMERS)
    return (header + bins + "[dependencies]\ncpp_sample_runtime_core={package=" + json.dumps(package)
            + ",path=" + json.dumps(str(sample / "lib")) + "}\n" + dependencies)


def payload(name):
    return "".join(json.dumps(row["input"], ensure_ascii=True) + "\n" for row in corpus(name)["cases"])


def execute(path):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh JSON receipt and owned output space")
    evaluation()
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  worker_source=WORKER.read_text(), inputs_before=execution_inputs(), references_before=references(),
                  native_inputs_before=native_inputs(), preparation_steps=[], steps=[], models=[], consumers=[])
    owned = None

    def run(label, command, *, env, input_text=None, expected=0):
        command = list(map(str, command))
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=input_text, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, input=input_text, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode == expected, label + " failed; inspect retained raw output")
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_json_consumers_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = temporary
            try:
                capabilities, classpath = prepare(owned, record)
                config = record["capabilities"]
                config["schema"] = "teaql.rust_configuration.job.v3"
                capabilities.write_text(json.dumps(config, sort_keys=True))
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                           CARGO_TARGET_DIR=str(owned / "consumer_target"),
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                           TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                originals, sdks = {}, {}
                for name, model in models().items():
                    directory = owned / name
                    directory.mkdir()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(model.encode()).decode()})
                    source, cpp, report = directory / "model.json", directory / "cpp", directory / "job.json.gz"
                    source.write_text(json.dumps(package, sort_keys=True))
                    row = dict(name=name, model=model, source_package=package, entry=MODEL_ENTRIES[name])
                    record["models"].append(row)
                    command = [config["cpp_generator"]["command"][0], "-cp", classpath,
                        "CppCompilerServiceProbe", source, cpp, Path(sys.executable).resolve(), WORKER,
                        capabilities, owned / "service_jobs", report]
                    if row["entry"] == "service":
                        run(name + "_controller_refusal", [*command, "--controller"], env=env, expected=1)
                        row.update(public_sdk_absent=not cpp.exists(), public_worker_report_absent=not report.exists())
                        require(row["public_sdk_absent"] and row["public_worker_report_absent"], "public refusal precedes compiler dispatch")
                    run(name + "_" + row["entry"], command + (["--controller"] if row["entry"] == "controller" else []), env=env)
                    row.update(job=read(report), cpp_sources=hashes(cpp),
                               provenance=json.loads((cpp / "RUST_CONFIGURATION.json").read_bytes()),
                               service_jobs_remaining=list(map(str, (owned / "service_jobs").iterdir())),
                               compiler_jobs_remaining=list(map(str, (owned / "compiler_jobs").iterdir())))
                    raw = base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True)
                    require(json.loads(raw) == package, "same complete controller model bytes")
                    source.write_bytes(raw)
                    original, inventory = directory / "original", directory / "inventory.json"
                    run(name + "_original_generate", [*config["reference_generator"]["command"], source,
                        original, inventory, "--source-package", "--scalar-types"], env=env)
                    row.update(original_sources=hashes(original), original_inventory=json.loads(inventory.read_bytes()))
                    require(row["original_sources"] == row["job"]["original_sources_before"], "same complete original library for compiler and JSON consumers")
                    originals[name], sdks[name] = original, cpp
                harness = owned / "consumer"
                harness.mkdir()
                packages = {row["name"]: row["original_inventory"]["package"] for row in record["models"]}
                text = cargo_manifest(originals, packages)
                (harness / "Cargo.toml").write_text(text)
                shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                run("original_consumer_resolve", ["cargo", "metadata", "--offline", "--format-version", "1",
                    "--manifest-path", harness / "Cargo.toml"], env=env)
                require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "reference consumer dependency identities")
                run("original_consumers_build", ["cargo", "build", "--offline", "--locked", "--bins", "-j", "2",
                    "--manifest-path", harness / "Cargo.toml"], env=env)
                record.update(consumer_manifest=text, consumer_lock=(harness / "Cargo.lock").read_text())
                binaries = {}
                for name in CONSUMERS:
                    adapted, edits = adapted_cpp(name, sdks)
                    cpp_source, native = owned / (name + ".cpp"), owned / ("native_" + name)
                    cpp_source.write_text(adapted)
                    row = dict(name=name, cpp_source=consumer_sources(name)[0].read_text(),
                               rust_source=consumer_sources(name)[1].read_text(), adapted_source=adapted, include_edits=edits,
                               corpus=corpus(name))
                    record["consumers"].append(row)
                    command = native_command(sdks["typed_member_aliases"] / "include", cpp_source, native)
                    command[1:1] = ["-I" + str(PARITY), "-I" + str(sdks["sample_fields_runtime"] / "include")]
                    run(name + "_native_build", command, env=env)
                    data = payload(name)
                    original = owned / "consumer_target/debug" / ("json_" + name)
                    row["original"] = json_lines(run(name + "_original_run", [original], env=env, input_text=data))
                    row["cpp"] = json_lines(run(name + "_native_run", [native], env=env, input_text=data))
                    expected = [case["expected"] for case in row["corpus"]["cases"]]
                    require(same(row["original"], expected) and same(row["cpp"], expected), "actual JSON inputs differ: " + name)
                    binaries[name] = (original, native)
                original, native = binaries["inputs"]
                data = '{"case": "payload_diagnostic"}\n'
                record["original_payload_diagnostic"] = json_lines(run("approved_json_original_diagnostic", [original], env=env, input_text=data))
                data = '{"case": "combined"}\n'
                run("approved_json_original_combined", [original], env=env, input_text=data, expected=101)
                record["cpp_combined"] = json_lines(run("approved_json_cpp_combined", [native], env=env, input_text=data))
                _, native = binaries["owned"]
                run("owned_shape_lifetime", [native, "--shape-cache"], env=env)
                for row in record["models"]:
                    row.update(original_sources_after=hashes(originals[row["name"]]), cpp_sources_after=hashes(sdks[row["name"]]))
                record["artifacts_after"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                record["complete"] = True
            finally:
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


def validate(record):
    evaluation()
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["driver_source"] == Path(__file__).read_text() and record["worker_source"] == WORKER.read_text(), "complete exact compiler JSON execution")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and record["inputs_before"] == record["inputs_after"] == execution_inputs()
            and record["references_before"] == record["references_after"] == references()
            and record["native_inputs_before"] == record["native_inputs_after"] == native_inputs(), "stable exact sources/references/native inputs and cleanup")
    require(record["capabilities"]["schema"] == "teaql.rust_configuration.job.v3"
            and [s["label"] for s in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
            and all(type(s["exit_code"]) is int and s["exit_code"] == 0 for s in record["preparation_steps"]), "two fresh classpaths and explicit scalar capability")
    prepared = {name for step in record["preparation_steps"] for name in step["command"] if name.endswith(".java")}
    require(set(record["prepared_java_sources"]) == prepared
            and all(sha(Path(name).read_bytes()) == value for name, value in record["prepared_java_sources"].items()), "all actual compiled Java source identities")
    require([row["name"] for row in record["models"]] == list(MODEL_NAMES), "complete authentic model inventory")
    worker = worker_module()
    labels = []
    sdks, originals = {}, {}
    for row in record["models"]:
        name, job = row["name"], row["job"]
        directory = Path(record["owned_directory"]) / name
        sdks[name], originals[name] = directory / "cpp", directory / "original"
        require(row["model"] == models()[name] and row["entry"] == MODEL_ENTRIES[name]
                and row["source_package"] == dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                    files={"main.xml": base64.b64encode(row["model"].encode()).decode()}), "unchanged full model source")
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(json.loads(raw) == row["source_package"] and sha(raw) == job["model_sha256"]
                and job["worker_sha256"] == sha(record["worker_source"].encode())
                and job["capabilities_sha256"] == sha(json.dumps(record["capabilities"], sort_keys=True).encode()), "actual model, worker and capability binding")
        require(job["complete"] is True and job["outcome"] == "resolved"
                and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and row["service_jobs_remaining"] == row["compiler_jobs_remaining"] == [], "complete compiler/controller execution and cleanup")
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        worker.validate_scalar_types(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
        document = job["resolution_document"]
        require(document["schema"] == "teaql.rust_configuration.model_generation.v2"
                and document["scalar_types"] == job["scalar_types"]
                and document["scalar_observer_sha256"] == sha(job["scalar_observer_source"].encode())
                and job["scalar_observer_source"] == worker.scalar_source(job["original_inventory"]["entities"])
                and document["scalar_artifacts_sha256"] == worker.manifest_sha(job["scalar_artifacts"])
                and job["scalar_artifacts"] == job["configuration_artifacts"] == job["observer_artifacts"], "same-library scalar/descriptor/MIR identities")
        require(row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"]
                and row["original_inventory"] == job["original_inventory"]
                and row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"]
                and row["provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode()), "complete immutable original/C++ SDKs and provenance")
        steps = {step["label"]: step for step in job["steps"]}
        require(json.loads(steps["original_scalar_types"]["stdout"]) == job["scalar_types"]
                and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in job["steps"]), "all actual worker subprocesses terminate successfully")
        if row["entry"] == "service":
            require(row["public_sdk_absent"] is True and row["public_worker_report_absent"] is True, "reserved public model stays rejected")
            labels.append(name + "_controller_refusal")
        labels.extend([name + "_" + row["entry"], name + "_original_generate"])
    labels.extend(["original_consumer_resolve", "original_consumers_build"])
    require([row["name"] for row in record["consumers"]] == list(CONSUMERS), "all four unchanged consumers")
    steps = {step["label"]: step for step in record["steps"]}
    packages = {row["name"]: row["original_inventory"]["package"] for row in record["models"]}
    require(record["consumer_manifest"] == cargo_manifest(originals, packages)
            and identities(SimpleNamespace(read_text=lambda: record["consumer_lock"]))
                <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "complete consumer library/source/dependency binding")
    for row in record["consumers"]:
        name = row["name"]
        source, edits = adapted_cpp(name, sdks)
        require(row["cpp_source"] == consumer_sources(name)[0].read_text()
                and row["rust_source"] == consumer_sources(name)[1].read_text()
                and row["adapted_source"] == source and row["include_edits"] == edits
                and row["corpus"] == corpus(name), "exact consumer bodies, include-only adapter and complete original corpus")
        require(steps[name + "_original_run"]["input"] == steps[name + "_native_run"]["input"] == payload(name)
                and row["original"] == json_lines(steps[name + "_original_run"]["stdout"])
                and row["cpp"] == json_lines(steps[name + "_native_run"]["stdout"])
                and same(row["original"], [case["expected"] for case in row["corpus"]["cases"]])
                and same(row["cpp"], row["original"]), "all actual inputs/stdout and retained observations agree")
        labels.extend([name + "_native_build", name + "_original_run", name + "_native_run"])
    require(sha(DIAGNOSTIC.read_bytes()) == DIAGNOSTIC_SHA, "immutable approved JSON diagnostic")
    diagnostic = read(DIAGNOSTIC)["observations"]
    expected_diagnostic = json_lines(diagnostic[0]["rust"]["stdout"])
    expected_combined = json_lines(diagnostic[1]["cpp"]["stdout"])
    require(record["original_payload_diagnostic"] == json_lines(steps["approved_json_original_diagnostic"]["stdout"]) == expected_diagnostic
            and record["cpp_combined"] == json_lines(steps["approved_json_cpp_combined"]["stdout"])
            and same(record["cpp_combined"], expected_combined)
            and steps["approved_json_original_combined"]["stdout"] == ""
            and "JSON_ENTITY_INPUT: invalid field type: payload" in steps["approved_json_original_combined"]["stderr"], "approved JSON exception retains authentic refusal and C++ roundtrip")
    require(steps["approved_json_original_diagnostic"]["input"] == '{"case": "payload_diagnostic"}\n'
            and steps["approved_json_original_combined"]["input"] == steps["approved_json_cpp_combined"]["input"] == '{"case": "combined"}\n', "exact exception input membership")
    require(steps["owned_shape_lifetime"]["stdout"].splitlines()
            == ["JSON_CACHE_LIFETIME rows=" + str(count) + ";passed" for count in (1, 100, 10000)], "actual retained shape/session lifetime execution")
    labels.extend(["approved_json_original_diagnostic", "approved_json_original_combined", "approved_json_cpp_combined", "owned_shape_lifetime"])
    require([step["label"] for step in record["steps"]] == labels
            and all(type(step["exit_code"]) is int and step["exit_code"] == (
                    101 if step["label"] == "approved_json_original_combined" else 1 if step["label"].endswith("_controller_refusal") else 0)
                    for step in record["steps"]), "exact complete terminal process inventory")
    require(record["artifacts_after"] == record["capabilities"]["artifacts"], "compiled Java artifacts remain immutable")


def controls(record):
    validate(record)
    changes = (
        ("unfinished", lambda r: r.update(complete=False)),
        ("missing_model", lambda r: r["models"].pop()),
        ("changed_model", lambda r: r["models"][0].update(model="<root/>")),
        ("old_capability", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v2")),
        ("missing_scalar", lambda r: r["models"][0]["job"]["scalar_types"][0]["properties"].pop()),
        ("wrong_library", lambda r: r["models"][0]["job"]["scalar_artifacts"].update(invented="0" * 64)),
        ("changed_source", lambda r: r["models"][0]["original_sources"].update(invented="0" * 64)),
        ("wrong_decoder", lambda r: r["consumers"][0].update(cpp_source="int main(){}\n")),
        ("changed_function_body", lambda r: r["consumers"][0].update(adapted_source="int main(){}\n")),
        ("changed_rust_driver", lambda r: r["consumers"][0].update(rust_source="fn main(){}\n")),
        ("wrong_consumer_manifest", lambda r: r.update(consumer_manifest="[package]\nname=\"wrong\"\n")),
        ("missing_consumer", lambda r: r["consumers"].pop()),
        ("missing_case", lambda r: r["consumers"][1]["corpus"]["cases"].pop()),
        ("missing_actual_row", lambda r: r["consumers"][1]["cpp"].pop()),
        ("changed_raw_output", lambda r: next(s for s in r["steps"] if s["label"] == "nullable_native_run").update(stdout="{}\n")),
        ("wrong_input", lambda r: next(s for s in r["steps"] if s["label"] == "nullable_native_run").update(input="{}\n")),
        ("hidden_reference_refusal", lambda r: next(s for s in r["steps"] if s["label"] == "approved_json_original_combined").update(exit_code=0)),
        ("changed_exception", lambda r: r.update(cpp_combined=[])),
        ("lost_shape_lifetime", lambda r: next(s for s in r["steps"] if s["label"] == "owned_shape_lifetime").update(stdout="")),
        ("boolean_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("missing_process", lambda r: r["steps"].pop()),
        ("missing_cleanup", lambda r: r.update(owned_directory_removed=False)),
    )
    result = []
    for name, mutate in changes:
        bad = copy.deepcopy(record)
        mutate(bad)
        try:
            validate(bad)
        except (RuntimeError, KeyError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler JSON corruption: " + name)
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
            require(not args.controls.exists(), "preserve earlier JSON controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                  execution_sha256=sha(args.record.read_bytes()), controls=result))
        print("Verified complete compiler JSON consumers:", len(result), "controls; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
