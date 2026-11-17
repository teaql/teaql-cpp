#!/usr/bin/env python3
"""Carry composed model configurations through Spring-hosted governed SQLite execution."""

import argparse
import base64
import copy
import json
import os
from pathlib import Path
import sys
import tempfile

import verify_primitive_const_governed_execution_5_0_6 as engine
from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, read, references, require, sha, write
from verify_compiler_governed_execution_5_0_6 import maven_command, prepare
from verify_compiler_host_assembly_5_0_6 import HOST, WORKER
from verify_compiler_source_service_5_0_6 import artifact_sha

ROOT = Path(__file__).resolve().parents[1]
MAIN = "com.skynet.codegenerator.cpp.CppCoreGenerator"


def execute(case_name, record_path, sink_path):
    require(not record_path.exists() and not sink_path.exists(), "fresh governed host execution and sink records")
    adapter = dict(host_adapter_source=Path(__file__).read_text(), host_adapter_sha256=sha(Path(__file__).read_bytes()),
                   worker_source=WORKER.read_text(), host_probe_source=HOST.read_text(),
                   preparation_steps=[], host_preparation_steps=[], process_adaptations=[],
                   host_inputs_before=execution_inputs(), host_references_before=references(),
                   host_native_before=native_inputs())
    original_run = engine.run_owned
    owned = None
    error = None
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_host_governed_") as directory:
            owned = Path(directory)
            adapter["host_directory"] = directory
            capabilities, classpath = prepare(owned, adapter)
            config = adapter["capabilities"]
            config["schema"] = "teaql.rust_configuration.job.v4"
            command = ["javac", "--release", "17", "-cp", classpath, "-d", str(owned / "cpp_classes"), str(HOST)]
            result = original_run(command, cwd=ROOT, text=True, capture_output=True, timeout=900)
            adapter["host_preparation_steps"].append(dict(command=command, exit_code=result.returncode,
                                                         stdout=result.stdout, stderr=result.stderr))
            require(result.returncode == 0, "fresh Spring host probe compilation")
            config["artifacts"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
            capabilities.write_text(json.dumps(config, sort_keys=True))
            generated = False

            def run(command, **kwargs):
                nonlocal generated
                source = list(map(str, command))
                actual = list(source)
                changes = {}
                env = kwargs.get("env")
                linker = "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"
                if env is not None and linker in env:
                    require(os.environ.get("CC"), "activate the locked native compiler")
                    changes[linker] = dict(before=env[linker], after=os.environ["CC"])
                    kwargs["env"] = dict(env, **{linker: os.environ["CC"]})
                if actual[0] == "mvn":
                    actual[0] = maven_command()
                if MAIN in source:
                    require(not generated and source[-3] == MAIN, "exactly one configured generation replacement")
                    generated = True
                    model, output = map(Path, source[-2:])
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(model.read_bytes()).decode()})
                    packaged = owned / "model.json"
                    packaged.write_text(json.dumps(package, sort_keys=True))
                    report = owned / "compiler_report.json.gz"
                    actual = [config["cpp_generator"]["command"][0], "-cp", classpath, "CppCompilerHostProbe",
                              str(packaged), str(output), str(Path(sys.executable).resolve()), str(WORKER),
                              str(capabilities), str(owned / "service_jobs"), str(report), "--controller"]
                    adapter["host_source_package"] = package
                adapter["process_adaptations"].append(dict(source_command=source, executed_command=actual,
                                                          environment_changes=changes))
                result = original_run(actual, **kwargs)
                if MAIN in source:
                    adapter["host_process"] = dict(command=actual, exit_code=result.returncode,
                                                   stdout=result.stdout, stderr=result.stderr)
                    if report.is_file():
                        adapter.update(host_job=read(report),
                            host_service_jobs_remaining=list(map(str, (owned / "service_jobs").iterdir())),
                            host_compiler_jobs_remaining=list(map(str, (owned / "compiler_jobs").iterdir())),
                            host_artifacts_after={name: artifact_sha(Path(name)) for name in config["artifacts"]})
                    if result.returncode == 0:
                        adapter["host_cpp_provenance"] = json.loads((output / "RUST_CONFIGURATION.json").read_bytes())
                return result

            engine.run_owned = run
            try:
                engine.execute(case_name, record_path, sink_path)
            finally:
                engine.run_owned = original_run
                adapter["host_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
            require(generated, "actual governed model uses the configured Spring host")
    except BaseException as caught:
        error = caught
        raise
    finally:
        engine.run_owned = original_run
        adapter.update(host_directory_removed=owned is not None and not owned.exists(),
                       host_inputs_after=execution_inputs(), host_references_after=references(),
                       host_native_after=native_inputs())
        record = read(record_path) if record_path.is_file() else dict(complete=False, whole_goal_complete=False)
        record.update(adapter)
        record["complete"] = record.get("complete") is True and error is None
        if error is not None:
            record["host_adapter_error"] = repr(error)
        write(record_path, record)
    record = read(record_path)
    require(record["host_inputs_before"] == record["host_inputs_after"]
            and record["host_references_before"] == record["host_references_after"]
            and record["host_native_before"] == record["host_native_after"], "frozen governed host execution inputs")
    require(record["host_job"]["complete"] is True and record["host_job"]["outcome"] == "resolved"
            and record["host_job"]["cpp_sources"] == record["cpp_generated_hashes"]
            and record["host_service_jobs_remaining"] == record["host_compiler_jobs_remaining"] == [],
            "actual governed consumer uses the complete host SDK and cleaned jobs")
    print("Spring-hosted governed model and sink execution complete:", case_name, flush=True)


def validate(record, sinks):
    from compiler_host_governance_source_admission_5_0_6 import bound, frame_matches, registered_epoch_matches
    from verify_compiler_model_configuration_5_0_6 import worker_module
    engine.validate(record, sinks)
    name = record["case"]["name"]
    actual = bound(name)
    require(registered_epoch_matches(Path(__file__), sha(record["host_adapter_source"].encode()))
            and record["host_adapter_sha256"] == sha(record["host_adapter_source"].encode())
            and record["worker_source"] == WORKER.read_text()
            and registered_epoch_matches(HOST, sha(record["host_probe_source"].encode())),
            "actual host adapter and unchanged worker/probe")
    require(record["host_inputs_before"] == record["host_inputs_after"]
            and record["host_inputs_after"].keys() == actual["host_inputs_after"].keys()
            and frame_matches(execution_inputs(), record["host_inputs_after"])
            and record["host_references_before"] == record["host_references_after"] == references()
            and record["host_native_before"] == record["host_native_after"] == native_inputs(),
            "complete frozen source, reference and native host inputs")
    require(record["host_directory_removed"] is True and not Path(record["host_directory"]).exists()
            and record["host_service_jobs_remaining"] == record["host_compiler_jobs_remaining"] == [],
            "all host service/compiler output removed")
    require(record["capabilities"] == actual["capabilities"]
            and record["capabilities"]["schema"] == "teaql.rust_configuration.job.v4"
            and record["host_artifacts_after"] == record["capabilities"]["artifacts"], "exact actual semantic capability and immutable artifacts")
    for key in ("preparation_steps", "host_preparation_steps"):
        require(len(record[key]) == len(actual[key])
                and all(step["command"] == executed["command"] and type(step["exit_code"]) is int
                        and step["exit_code"] == 0 for step, executed in zip(record[key], actual[key], strict=True)),
                "all exact fresh Java host/preparation commands")
    require(record["prepared_java_sources"] == actual["prepared_java_sources"]
            and all(registered_epoch_matches(Path(path), digest) for path, digest in record["prepared_java_sources"].items()),
            "complete actual prepared Java sources")
    require(record["host_source_package"] == dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
            files={"main.xml": base64.b64encode(record["model_source"].encode()).decode()}), "complete unchanged business model through controller upload")
    config = record["capabilities"]
    root = Path(record["host_directory"])
    base = config["cpp_generator"]["command"]
    classpath = base[base.index("-cp") + 1]
    steps = {step["label"]: step for step in record["steps"]}
    selected = 0
    require(len(record["process_adaptations"]) == len(actual["process_adaptations"]), "complete actual process adaptation membership")
    require([step["command"] for step in record["steps"]]
            == [adaptation["source_command"] for adaptation in record["process_adaptations"]],
            "every recorded business step binds its actual adapted invocation")
    for adaptation, executed in zip(record["process_adaptations"], actual["process_adaptations"], strict=True):
        source = adaptation["source_command"]
        expected = list(source)
        if expected[0] == "mvn":
            expected[0] = maven_command()
        if MAIN in source:
            selected += 1
            expected = [base[0], "-cp", classpath, "CppCompilerHostProbe", str(root / "model.json"), source[-1],
                        str(Path(sys.executable).resolve()), str(WORKER), str(root / "capabilities.json"),
                        str(root / "service_jobs"), str(root / "compiler_report.json.gz"), "--controller"]
        require(adaptation["source_command"] == executed["source_command"]
                and adaptation["executed_command"] == expected
                and adaptation["environment_changes"] == executed["environment_changes"], "only actual declared host, Maven and linker adaptations")
    require(selected == 1, "one and only one Spring-host generation call")
    process = record["host_process"]
    require(process["command"] == next(a["executed_command"] for a in record["process_adaptations"] if MAIN in a["source_command"])
            and type(process["exit_code"]) is int and process["exit_code"] == steps["cpp_generate"]["exit_code"] == 0
            and process["stdout"] == steps["cpp_generate"]["stdout"]
            and process["stderr"] == steps["cpp_generate"]["stderr"], "actual Spring process output, without fabricated generation success")
    job = record["host_job"]
    worker = worker_module()
    raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
    require(job["complete"] is True and job["outcome"] == "resolved"
            and json.loads(raw) == record["host_source_package"] and sha(raw) == job["model_sha256"]
            and job["capabilities_sha256"] == sha(json.dumps(config, sort_keys=True).encode())
            and job["worker_sha256"] == sha(record["worker_source"].encode())
            and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
            and not Path(job["owned_directory"]).exists() and not Path(job["publication_directory"]).exists(),
            "actual complete bound compiler job and cleanup")
    require(job["original_sources_before"] == job["original_sources_after"]
            and job["references_before"] == job["references_after"] == {
                "reference_generator": worker.GENERATOR, "runtime_reference": worker.RUNTIME}, "unchanged complete original SDK and references")
    worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
    worker.validate_semantic_storage(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
    document = job["resolution_document"]
    require(document["schema"] == "teaql.rust_configuration.model_generation.v3"
            and document["entities"] == job["configuration"] and document["scalar_types"] == job["scalar_types"]
            and document["configuration_observer_sha256"] == sha(job["configuration_observer_source"].encode())
            and document["scalar_observer_sha256"] == sha(job["scalar_observer_source"].encode())
            and document["configuration_artifacts_sha256"] == worker.manifest_sha(job["configuration_artifacts"])
            and document["scalar_artifacts_sha256"] == worker.manifest_sha(job["scalar_artifacts"])
            and job["configuration_artifacts"] == job["scalar_artifacts"] == job["observer_artifacts"], "same original library supplies descriptors, semantic storage and projections")
    projection = document["projection_resolution"]
    require(projection["source_manifest"] == job["original_sources_before"]
            and projection["model_sha256"] == job["model_sha256"]
            and projection["mir_sha256"] == job["mir_sha256"] == sha(job["mir"].encode())
            and projection["dependency_lock_sha256"] == sha(job["dependency_lock"].encode())
            and projection["compiler_environment_sha256"] == job["compiler_environment_sha256"], "actual source, MIR, dependencies and compiler environment")
    require(record["cpp_generated_hashes"] == job["cpp_sources"]
            and record["host_cpp_provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode()),
            "the compiled business SDK is the complete actual host SDK")
    compiled = {step["label"]: step for step in job["steps"]}
    require(all(type(compiled[key]["exit_code"]) is int and compiled[key]["exit_code"] == 0 for key in (
        "original_compile", "observer_compile", "original_descriptors", "configuration_observer_compile", "original_configuration",
        "scalar_observer_compile", "original_scalar_types", "cpp_generate")), "actual complete compiler/observer/emitter processes")
    school = next(entity for entity in job["configuration"] if entity["name"] == "School")
    require(school["limit"] == record["case"]["limit"] and school["masks"] == ["name"], "compiled metadata reaches actual protected Save and publication")


def controls(record, sinks):
    validate(record, sinks)
    result = engine.controls(record, sinks)
    mutations = (
        ("host_source", lambda r: r.update(host_adapter_source="forged")),
        ("empty_host_frames", lambda r: r.update(host_inputs_before={}, host_inputs_after={})),
        ("old_capability", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v2")),
        ("lost_java_source", lambda r: r["prepared_java_sources"].pop(next(iter(r["prepared_java_sources"])))),
        ("lost_preparation", lambda r: r["host_preparation_steps"].pop()),
        ("substituted_java", lambda r: r["host_preparation_steps"][0].update(command=["true"])),
        ("lost_process", lambda r: r["process_adaptations"].pop()),
        ("substituted_host", lambda r: r["host_process"].update(command=["true"])),
        ("substituted_business_step", lambda r: r["steps"][10].update(command=["true"])),
        ("boolean_host_exit", lambda r: r["host_process"].update(exit_code=False)),
        ("wrong_model", lambda r: r["host_source_package"].update(files={})),
        ("hidden_refusal", lambda r: r["host_job"].update(outcome="compiler_refusal")),
        ("missing_compiled_property", lambda r: r["host_job"]["configuration"][0]["properties"].pop()),
        ("wrong_mir", lambda r: r["host_job"].update(mir_sha256="0" * 64)),
        ("wrong_library", lambda r: r["host_job"]["scalar_artifacts"].update(forged="0" * 64)),
        ("wrong_sdk", lambda r: r["host_job"]["cpp_sources"].update(forged="0" * 64)),
        ("live_job", lambda r: r["host_service_jobs_remaining"].append("live")),
        ("cleanup", lambda r: r.update(host_directory_removed=False)),
    )
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed, sinks)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted governed host corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("complete", "partial"), required=True)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--sink-record", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.check:
        record, sinks = read(args.record), read(args.sink_record)
        require(record["case"]["name"] == args.case, "requested actual composed case")
        validate(record, sinks)
        if args.controls:
            require(not args.controls.exists(), "preserve previous governed host controls")
            result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                          case=args.case, controls=controls(record, sinks))
            write(args.controls, result)
            print("Governed host verified;", len(result["controls"]), "corruptions rejected; optimized=", sys.flags.optimize)
        else:
            print("Verified actual Spring-hosted governed model:", args.case)
    else:
        execute(args.case, args.record.resolve(), args.sink_record.resolve())


if __name__ == "__main__":
    main()
