#!/usr/bin/env python3
"""Run complete generated E/JSON and Checker/Fix consumers through the controller."""

import argparse
import base64
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_governed_execution_5_0_6 import prepare, prerequisites
from verify_compiler_model_configuration_5_0_6 import worker_module
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_source_service_5_0_6 import artifact_sha
from verify_primitive_const_governed_execution_5_0_6 import run_owned
import verify_ap02_reverse_relation_sql_5_0_6 as expressions

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
RUNTIME = ROOT / ".reference/teaql-rs"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
LOCK = ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"
CHECKER_CPP = ROOT / "tests/parity/generated_checker_flow_probe.cpp"
CHECKER_RUST = ROOT / "tests/parity/rust_field_layout/src/bin/generated_checker_flow_5_0_6.rs"
CHECKER_INCLUDE = '#include "../generated_checkers/include/cpp_checker_fixture.hpp"'
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same


def require(condition, message):
    if not condition:
        raise RuntimeError("Compiler-generated business: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, record):
    path.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))


def execution_inputs():
    # The append-only human acceptance ledger does not enter generation or execution.
    return {name: value for name, value in capture_inputs().items()
            if name != "runtime:docs/cpp_generator_parity_ledger.json"}


def native_inputs():
    paths = [*sorted((ROOT / "build_native_current").glob("teaql_*/libteaql_*.a")),
             Path(native_command(ROOT, ROOT / "probe.cpp", ROOT / "probe")[0]),
             ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a"]
    require(len(paths) >= 8, "complete six-library native runtime, compiler and SQLite inputs")
    return {str(path): sha(path.read_bytes()) for path in paths}


def cases():
    _, _, selected = expressions.selected_cases()
    result = []
    for row in selected:
        original = expressions.model(row["input"])
        marker = '<child _parent="item"'
        require(original.count(marker) == 1, "one original inherited secret owner")
        model = original.replace(marker, marker + ' _audit_mask_fields="secret"')
        result.append(dict(name=row["name"], kind="expression", model=model, many=row["many"],
                           original_model=original, derivation=dict(attribute="child._audit_mask_fields",
                           before=None, after="secret", rule="KSML-PRIVACY-001-ERR")))
    result.append(dict(name="checker", kind="checker",
                       model=(ROOT / "tests/generator/checkers.xml").read_text()))
    return result


def inputs(case):
    if case["kind"] == "expression":
        return [dict(name=name, parent=parent, selected=selected, minimal=minimal, many=case["many"])
                for name, parent, selected, minimal in (
                    ("unselected", 10, False, False), ("loaded", 10, True, False),
                    ("empty", 11, True, False), ("minimal", 10, True, True),
                    ("no_parent", 99, True, False))]
    return [row["input"] for name in ("flow", "commit")
            for row in read(ROOT / ("tests/parity/generated_checker_" + name + "_5_0_6_cases.json.gz"))["cases"]]


def checker_source():
    source = CHECKER_CPP.read_text()
    require(source.count(CHECKER_INCLUDE) == 1, "one explicit retained generated-header include")
    return source.replace(CHECKER_INCLUDE, '#include "cpp_checker_fixture.hpp"')


def manifest(original, alias, source, package=None):
    if package is None:
        package = re.search(r'^name\s*=\s*"([^"]+)"',
                            (original / "lib/Cargo.toml").read_text(), re.M)[1]
    return ('[package]\nname="compiler_generated_business_probe"\nversion="0.1.0"\n'
            'edition="2024"\n[workspace]\n[[bin]]\nname="compiler_generated_business_probe"\npath='
            + json.dumps(str(source)) + '\n[dependencies]\n' + alias + '={package='
            + json.dumps(package) + ',path=' + json.dumps(str(original / "lib")) + '}\n'
            'serde_json="=1.0.150"\nrusqlite="=0.40.1"\nteaql-core="5.0.6"\n'
            'teaql-runtime="5.0.6"\nteaql-provider-sqlite="5.0.6"\n[patch.crates-io]\n'
            + "".join(name + '={path=' + json.dumps(str(RUNTIME / name)) + '}\n'
                      for name in expressions.PACKAGES))


def execute(destination):
    require(not destination.exists(), "preserve earlier execution receipts")
    require(shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "space for owned compiler and consumer outputs")
    prerequisites()
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  input_hashes_before=execution_inputs(), references_before=references(),
                  preparation_steps=[], steps=[], cases=[], worker_source=WORKER.read_text())
    owned = None

    def run(label, command, *, env=None, input_text=None, cwd=ROOT):
        command = list(map(str, command))
        print(label, "running", flush=True)
        result = run_owned(command, cwd=cwd, env=env, input=input_text, text=True,
                           capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, cwd=str(cwd),
                                    input=input_text, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(destination, record)
        print(label, "exit", result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_generated_business_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capabilities, classpath = prepare(owned, record)
                config = record["capabilities"]
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                           CARGO_TARGET_DIR=str(owned / "consumer_target"),
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                           TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                record["rust_environment"] = {key: value for key, value in env.items()
                                               if key.startswith("CARGO_") or key == "TEAQL_LOG_ENDPOINT"}
                record["native_inputs_before"] = native_inputs()
                for case in cases():
                    name = case["name"]
                    current = owned / name
                    current.mkdir()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(case["model"].encode()).decode()})
                    packaged = current / "model.json"
                    packaged.write_text(json.dumps(package, sort_keys=True))
                    cpp, original, report = current / "cpp", current / "original", current / "compiler.json.gz"
                    row = dict(case, source_package=package)
                    record["cases"].append(row)
                    run(name + "_controller", [config["cpp_generator"]["command"][0], "-cp", classpath,
                        "CppCompilerServiceProbe", packaged, cpp, Path(sys.executable).resolve(),
                        WORKER, capabilities, owned / "service_jobs", report, "--controller"], env=env)
                    row.update(job=read(report), cpp_provenance=json.loads((cpp / "RUST_CONFIGURATION.json").read_bytes()),
                               cpp_provenance_text=(cpp / "RUST_CONFIGURATION.json").read_text(),
                               cpp_sources=hashes(cpp), service_jobs_remaining=list(map(str, (owned / "service_jobs").iterdir())),
                               compiler_jobs_remaining=list(map(str, (owned / "compiler_jobs").iterdir())))
                    # The controller serializes the source envelope. Use those exact bytes
                    # for the independent reference so its byte-level inventory also matches.
                    raw_package = base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True)
                    require(json.loads(raw_package) == package, "controller preserves every model source byte")
                    packaged.write_bytes(raw_package)
                    row["reference_package_base64"] = base64.b64encode(raw_package).decode()
                    run(name + "_original_generate", [*config["reference_generator"]["command"],
                        packaged, original, current / "inventory.json", "--source-package"], env=env)
                    row["original_sources"] = hashes(original)
                    row["original_inventory"] = json.loads((current / "inventory.json").read_bytes())
                    require(row["original_sources"] == row["job"]["original_sources_before"],
                            "business reference SDK matches the compiler job's complete original library")
                    if case["kind"] == "expression":
                        cpp_source, rust_source, alias = expressions.CPP, expressions.RUST, "domain"
                    else:
                        cpp_source, rust_source, alias = current / "checker_probe.cpp", CHECKER_RUST, "cpp_checker_fixture_core"
                        cpp_source.write_text(checker_source())
                        row["cpp_include_adapter"] = cpp_source.read_text()
                    binary = owned / "native_probe"
                    command = native_command(cpp / "include", cpp_source, binary)
                    command[1:1] = ["-I" + str(ROOT / "tests/parity"),
                                    '-DTEAQL_AP02_SQL_FIXTURE="' + str(expressions.SQL) + '"']
                    run(name + "_native_build", command, env=env)
                    harness = current / "harness"
                    harness.mkdir()
                    text = manifest(original, alias, rust_source)
                    (harness / "Cargo.toml").write_text(text)
                    shutil.copyfile(RUNTIME / "Cargo.lock", harness / "Cargo.lock")
                    run(name + "_original_resolve", ["cargo", "metadata", "--offline", "--format-version", "1",
                        "--manifest-path", harness / "Cargo.toml"], env=env)
                    require(identities(harness / "Cargo.lock") <= identities(RUNTIME / "Cargo.lock"),
                            "consumer dependency versions and checksums are reference identities")
                    run(name + "_original_build", ["cargo", "build", "--offline", "--locked", "--manifest-path",
                        harness / "Cargo.toml", "-j", "2"], env=env)
                    payload = "".join(json.dumps(value, ensure_ascii=True) + "\n" for value in inputs(case))
                    reference_binary = owned / "consumer_target/debug/compiler_generated_business_probe"
                    original_values = json_lines(run(name + "_original_run", [reference_binary], env=env, input_text=payload))
                    native_values = json_lines(run(name + "_native_run", [binary], env=env, input_text=payload))
                    require(len(original_values) == len(native_values) == len(inputs(case)), "complete actual consumer result membership")
                    row.update(original=original_values, cpp=native_values, consumer_manifest=text,
                               consumer_lock=(harness / "Cargo.lock").read_text(),
                               binaries={str(path): sha(path.read_bytes()) for path in (binary, reference_binary)},
                               original_sources_after=hashes(original), cpp_sources_after=hashes(cpp))
                    require(row["original_sources"] == row["original_sources_after"]
                            and row["cpp_sources"] == row["cpp_sources_after"], "SDKs unchanged during compile and execution")
                    row["differences"] = [index for index, (left, right) in enumerate(zip(original_values, native_values, strict=True))
                                          if not same(left, right)]
                    write(destination, record)
                    require(not row["differences"], name + " actual original/native business outcomes differ")
                record["artifacts_after"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                record["native_inputs_after"] = {name: sha(Path(name).read_bytes()) for name in record["native_inputs_before"]}
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
    except BaseException as error:
        record.update(complete=False, execution_error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(),
                      input_hashes_after=execution_inputs(), references_after=references())
        write(destination, record)
    try:
        validate(record)
    except BaseException as error:
        record.update(complete=False, validation_error=repr(error))
        write(destination, record)
        raise


def validate(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["driver_source"] == Path(__file__).read_text()
            and record["worker_source"] == WORKER.read_text(), "completed exact execution implementation")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "all owned outputs removed")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"] == references(), "unchanged source/reference execution frame")
    frame = execution_inputs()
    require(all(frame.get(name) == value for name, value in record["input_hashes_after"].items()), "actual current source identities")
    require(record["native_inputs_before"] == record["native_inputs_after"] == native_inputs(),
            "actual native compiler, libraries and SQLite inputs")
    prepared = record["preparation_steps"]
    require([step["label"] for step in prepared] == ["original_bridge_javac", "cpp_javac"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in prepared), "independent fresh Java compilation")
    java = {name for step in prepared for name in step["command"] if name.endswith(".java")}
    require(set(record["prepared_java_sources"]) == java
            and all(sha(Path(name).read_bytes()) == value for name, value in record["prepared_java_sources"].items()), "complete compiled Java input inventory")
    config = record["capabilities"]
    worker = worker_module()
    require(config["schema"] == "teaql.rust_configuration.job.v2"
            and config["artifacts"] == record["artifacts_after"], "explicit full-metadata capability and unchanged Java artifacts")
    require([row["name"] for row in record["cases"]] == [row["name"] for row in cases()], "every original model in order")
    expected_labels = []
    for row, case in zip(record["cases"], cases(), strict=True):
        name = case["name"]
        require(all(row[key] == value for key, value in case.items()), "unchanged original business model")
        package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                       files={"main.xml": base64.b64encode(case["model"].encode()).decode()})
        require(row["source_package"] == package and row["service_jobs_remaining"] == row["compiler_jobs_remaining"] == [], "controller upload package and no outstanding jobs")
        job = row["job"]
        require(job["complete"] is True and job["outcome"] == "resolved"
                and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and not Path(job["owned_directory"]).exists() and not Path(job["publication_directory"]).exists(), "actual compiler job completion and cleanup")
        require(job["worker_sha256"] == sha(WORKER.read_bytes()) and job["artifacts"] == config["artifacts"]
                and job["capabilities_sha256"] == sha(json.dumps(config, sort_keys=True).encode()), "compiler capabilities and worker binding")
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(json.loads(raw) == package and sha(raw) == job["model_sha256"]
                and row["reference_package_base64"] == job["source_package_bytes_base64"], "same exact compiler and independent reference model input")
        require(row["original_inventory"] == job["original_inventory"]
                and row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"],
                "consumer original library is the unchanged compiler original library")
        require(row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"], "executed native SDK is the actual emitted compiler SDK")
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        document = job["resolution_document"]
        projection = document["projection_resolution"]
        require(document["schema"] == "teaql.rust_configuration.model_generation.v1"
                and same(document["entities"], job["configuration"])
                and document["configuration_observer_sha256"] == sha(job["configuration_observer_source"].encode())
                and job["configuration_observer_source"] == worker.configuration_source(job["original_inventory"]["entities"])
                and document["configuration_artifacts_sha256"] == worker.manifest_sha(job["configuration_artifacts"])
                and job["configuration_artifacts"] == job["observer_artifacts"], "metadata and projections from the same compiled original library")
        require(projection["model_sha256"] == job["model_sha256"]
                and projection["source_manifest"] == job["original_sources_before"]
                and projection["mir_sha256"] == job["mir_sha256"] == sha(job["mir"].encode())
                and projection["compiler_lock_sha256"] == sha(LOCK.read_bytes())
                and projection["dependency_lock_sha256"] == sha(job["dependency_lock"].encode())
                and projection["compiler_environment_sha256"] == job["compiler_environment_sha256"], "MIR, source, lock and environment provenance")
        expected_refs = {"reference_generator": worker.GENERATOR, "runtime_reference": worker.RUNTIME}
        require(job["references_before"] == job["references_after"] == expected_refs
                and record["references_after"] == {"teaql-rs": worker.RUNTIME, "teaql_code_gen": worker.GENERATOR}, "exact locked reference commits")
        require(row["cpp_provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode())
                and row["cpp_provenance"] == json.loads(row["cpp_provenance_text"])
                and row["cpp_sources"]["RUST_CONFIGURATION.json"] == sha(row["cpp_provenance_text"].encode()),
                "actual emitted configuration provenance")
        job_steps = {step["label"]: step for step in job["steps"]}
        for label in ("original_compile", "observer_compile", "original_descriptors", "configuration_observer_compile", "original_configuration", "cpp_generate"):
            require(type(job_steps[label]["exit_code"]) is int and job_steps[label]["exit_code"] == 0, "actual compiler and observer completion")
        require(same(json.loads(job_steps["original_configuration"]["stdout"]), job["configuration"]), "metadata from real observer stdout")
        suffixes = ("controller", "original_generate", "native_build", "original_resolve", "original_build", "original_run", "native_run")
        expected_labels.extend(name + "_" + suffix for suffix in suffixes)
        steps = {step["label"]: step for step in record["steps"]}
        controller = steps[name + "_controller"]["command"]
        require("CppCompilerServiceProbe" in controller and controller[-1] == "--controller"
                and controller[-4:-1] == [str(Path(record["owned_directory"]) / "capabilities.json"),
                                           str(Path(record["owned_directory"]) / "service_jobs"),
                                           str(Path(record["owned_directory"]) / name / "compiler.json.gz")], "actual controller and configured compiler invocation")
        require("-I" + str(Path(record["owned_directory"]) / name / "cpp/include") in steps[name + "_native_build"]["command"], "native compile consumes the new SDK")
        require(row["original"] == json_lines(steps[name + "_original_run"]["stdout"])
                and row["cpp"] == json_lines(steps[name + "_native_run"]["stdout"]), "business results bind real executable stdout")
        payload = "".join(json.dumps(value, ensure_ascii=True) + "\n" for value in inputs(case))
        require(steps[name + "_original_run"]["input"] == steps[name + "_native_run"]["input"] == payload
                and len(row["original"]) == len(row["cpp"]) == len(inputs(case))
                and same(row["original"], row["cpp"]) and row["differences"] == [], "every actual business case matches without output normalization")
        current = Path(record["owned_directory"]) / name
        alias, rust_source = ("domain", expressions.RUST) if case["kind"] == "expression" else ("cpp_checker_fixture_core", CHECKER_RUST)
        require(row["consumer_manifest"] == manifest(current / "original", alias, rust_source, row["original_inventory"]["package"]), "consumer imports the complete new reference library")
        if case["kind"] == "checker":
            require(row["cpp_include_adapter"] == checker_source(), "only generated header include redirected; complete C++ bodies retained")
            expected = [item["expected"] for cohort in ("flow", "commit")
                        for item in read(ROOT / ("tests/parity/generated_checker_" + cohort + "_5_0_6_cases.json.gz"))["cases"]]
            require(same(row["original"], expected), "fresh generated Checker results also preserve every retained source observation")
    require([step["label"] for step in record["steps"]] == expected_labels
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["steps"]), "all real terminal process membership")


def controls(record):
    validate(record)
    changes = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("wrong_source", lambda r: r.update(driver_source="pass\n")),
        ("missing_model", lambda r: r["cases"].pop()),
        ("changed_xml", lambda r: r["cases"][0].update(model="<root/>")),
        ("bypassed_controller", lambda r: r["steps"][0]["command"].pop()),
        ("projection_only", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v1")),
        ("missing_java_input", lambda r: r["prepared_java_sources"].pop(next(iter(r["prepared_java_sources"])))),
        ("unfinished_compile", lambda r: r["cases"][0]["job"]["steps"].pop(next(i for i,s in enumerate(r["cases"][0]["job"]["steps"]) if s["label"] == "original_compile"))),
        ("wrong_metadata", lambda r: r["cases"][0]["job"]["configuration"].pop()),
        ("imported_descriptor_check", lambda r: r["cases"][0]["job"]["descriptors"].pop()),
        ("wrong_mir", lambda r: r["cases"][0]["job"].update(mir_sha256="0" * 64)),
        ("wrong_sdk", lambda r: r["cases"][0]["cpp_sources"].update(invented="0" * 64)),
        ("unreaped_job", lambda r: r["cases"][0]["compiler_jobs_remaining"].append("live")),
        ("missing_result", lambda r: r["cases"][0]["cpp"].pop()),
        ("changed_result", lambda r: r["cases"][0]["original"][0].update(error="invented")),
        ("changed_raw_stdout", lambda r: r["steps"][-1].update(stdout="{}\n")),
        ("changed_input", lambda r: r["steps"][-1].update(input="{}\n")),
        ("changed_checker_body", lambda r: r["cases"][-1].update(cpp_include_adapter="int main() {}\n")),
        ("changed_reference_package", lambda r: r["cases"][0].update(reference_package_base64="e30=")),
        ("changed_consumer_manifest", lambda r: r["cases"][0].update(consumer_manifest="[workspace]\n")),
        ("bool_exit", lambda r: r["steps"][-1].update(exit_code=False)),
        ("missing_process", lambda r: r["steps"].pop()),
        ("changed_source_frame", lambda r: r["input_hashes_after"].update(invented="0" * 64)),
    )
    result = []
    for name, mutate in changes:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (RuntimeError, KeyError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler business corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.record.resolve())
    else:
        result = controls(read(args.record))
        if args.controls:
            require(not args.controls.exists(), "preserve corruption receipts")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                                      execution_sha256=sha(args.record.read_bytes()), controls=result))
        print("Verified controller E/JSON/Checker consumers:", len(result), "controls; optimized=", sys.flags.optimize)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
