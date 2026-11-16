#!/usr/bin/env python3
"""Run the existing governed SQLite contract through compiled model generation."""

import argparse
import base64
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

import verify_primitive_const_governed_execution_5_0_6 as engine
from verify_compiler_model_configuration_5_0_6 import worker_module
from verify_compiler_source_service_5_0_6 import artifact_sha

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
SEED = ROOT / "docs/parity/compiler_model_configuration_full_5_0_6.json.gz"
SEED_SHA = "3d7a47edf73681f2c6e9700c5df0067fff8750eddcd94186053e3ca22350cef7"
ENGINE_SHA = "c889bf94646286acdcc80c54a84e1b3e6438b48afdfc77eb30ffc404b6131715"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
LOCK = ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"
MAIN = "com.skynet.codegenerator.cpp.CppCoreGenerator"


def require(value, message):
    if not value:
        raise RuntimeError("Compiled governed execution: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def maven_command():
    prior = ROOT / "docs/parity/ap02_primitive_const_path_installed_5_0_6.json.gz"
    path = Path(shutil.which("mvn") or read(prior)["steps"][0]["command"][0]).resolve()
    require(path.is_file(), "existing qualified Maven launcher")
    return str(path)


def prerequisites():
    from generator_evaluation_source_admission_5_0_6 import registered_epoch_matches
    require(sha(SEED.read_bytes()) == SEED_SHA, "immutable preparation recipe")
    require(registered_epoch_matches(Path(engine.__file__), ENGINE_SHA),
            "unchanged complete business execution and validation engine")
    return engine.prerequisites()


def prepare(directory, record):
    seed = read(SEED)
    old_directory = seed["owned_directory"]
    original_run = engine.run_owned
    for name in ("original_classes", "cpp_classes", "service_jobs", "compiler_jobs"):
        (directory / name).mkdir()
    for label in ("original_bridge_javac", "cpp_javac"):
        source = next(step["command"] for step in seed["steps"] if step["label"] == label)
        command = [part.replace(old_directory, str(directory)) for part in source]
        if label == "cpp_javac":
            for package in ("model", "generator"):
                for added in sorted((PRODUCER / package / "src/main/java").rglob("*.java")):
                    if str(added) not in command:
                        command.append(str(added))
        result = original_run(command, cwd=ROOT, text=True, capture_output=True, timeout=900)
        record["preparation_steps"].append(dict(label=label, command=command,
                                              exit_code=result.returncode,
                                              stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        print(label, "exit", result.returncode, flush=True)
    capabilities = copy.deepcopy(seed["capabilities"])
    classpaths = {}
    for key in ("reference_generator", "cpp_generator"):
        command = [part.replace(old_directory, str(directory))
                   for part in capabilities[key]["command"]]
        capabilities[key]["command"] = command
        classpaths[key] = command[command.index("-cp") + 1]
    capabilities["temporary_root"] = str(directory / "compiler_jobs")
    artifacts = {Path(capabilities["reference_generator"]["command"][0])}
    for classpath in classpaths.values():
        artifacts.update(map(Path, classpath.split(os.pathsep)))
    capabilities["artifacts"] = {str(path): artifact_sha(path) for path in sorted(artifacts)}
    require(capabilities["schema"] == "teaql.rust_configuration.job.v2",
            "explicit full-metadata capability selection")
    path = directory / "capabilities.json"
    path.write_text(json.dumps(capabilities, sort_keys=True))
    record.update(capabilities=capabilities, preparation_seed_sha256=SEED_SHA,
                  prepared_java_sources={name: sha(Path(name).read_bytes())
                                         for step in record["preparation_steps"] for name in step["command"]
                                         if name.endswith(".java")})
    return path, classpaths["cpp_generator"]


def execute(case_name, path, sinks, entry="service"):
    prerequisites()
    require(not path.exists() and not sinks.exists(), "preserve actual execution receipts")
    require(shutil.disk_usage(ROOT).free > 2 * 1024 ** 3, "owned output space")
    adapter = dict(adapter_source=Path(__file__).read_text(),
                   adapter_sha256=sha(Path(__file__).read_bytes()),
                   execution_engine_sha256=sha(Path(engine.__file__).read_bytes()), preparation_steps=[],
                   process_adaptations=[], worker_source=WORKER.read_text(), entry=entry)
    original_run = engine.run_owned
    error = None
    directory = None
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_governed_") as temporary:
            directory = Path(temporary)
            adapter["compiler_directory"] = temporary
            capabilities, classpath = prepare(directory, adapter)
            generated = False

            def run(command, **kwargs):
                nonlocal generated
                source = list(map(str, command))
                actual = list(source)
                environment_changes = {}
                env = kwargs.get("env")
                linker = "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"
                if env is not None and linker in env:
                    require(os.environ.get("CC"), "activate the locked native compiler")
                    environment_changes[linker] = dict(before=env[linker], after=os.environ["CC"])
                    kwargs["env"] = dict(env, **{linker: os.environ["CC"]})
                if actual[0] == "mvn":
                    actual[0] = maven_command()
                if MAIN in source:
                    require(not generated and source[-3] == MAIN, "one explicit generation replacement")
                    generated = True
                    model, output = map(Path, source[-2:])
                    packaged = directory / "model.json"
                    entrance = "main.xml" if entry == "controller" else model.name
                    source_package = dict(schema="teaql.model_source_package.v1",
                                          entrance_file=entrance,
                                          files={entrance: base64.b64encode(model.read_bytes()).decode()})
                    packaged.write_text(json.dumps(source_package, sort_keys=True))
                    report = directory / "compiler_report.json.gz"
                    actual = [source[0], "-cp", classpath, "CppCompilerServiceProbe", str(packaged),
                              str(output), str(Path(sys.executable).resolve()), str(WORKER),
                              str(capabilities), str(directory / "service_jobs"), str(report)]
                    if entry == "controller":
                        actual.append("--controller")
                    adapter.update(source_package=source_package, controller=entry == "controller")
                adaptation = dict(source_command=source, executed_command=actual,
                                  environment_changes=environment_changes)
                adapter["process_adaptations"].append(adaptation)
                result = original_run(actual, **kwargs)
                if MAIN in source:
                    adapter["compiler_service_process"] = dict(command=actual, exit_code=result.returncode,
                                                               stdout=result.stdout, stderr=result.stderr)
                    if not report.is_file() and result.returncode != 0:
                        return result
                    require(report.is_file(), "actual worker report before cleanup")
                    adapter.update(compiler_job=read(report),
                                   service_jobs_remaining=list(map(str, (directory / "service_jobs").iterdir())),
                                   compiler_jobs_remaining=list(map(str, (directory / "compiler_jobs").iterdir())),
                                   artifacts_after={name: artifact_sha(Path(name)) for name in adapter["capabilities"]["artifacts"]})
                    if result.returncode == 0:
                        adapter["cpp_provenance"] = json.loads((output / "RUST_CONFIGURATION.json").read_text())
                return result

            engine.run_owned = run
            try:
                engine.execute(case_name, path, sinks)
            finally:
                engine.run_owned = original_run
                adapter["compiler_bytes_removed"] = sum(p.stat().st_size for p in directory.rglob("*") if p.is_file())
            require(generated, "the business model used configured source generation")
    except BaseException as caught:
        error = caught
        raise
    finally:
        engine.run_owned = original_run
        adapter["compiler_directory_removed"] = directory is not None and not directory.exists()
        record = read(path) if path.exists() else dict(complete=False, whole_goal_complete=False)
        record.update(adapter)
        record["complete"] = record.get("complete") is True and error is None
        if error is not None:
            record["compiler_adapter_error"] = repr(error)
        write(path, record)
    record = read(path)
    record["compiler_validation_complete"] = True
    try:
        validate(record, read(sinks))
    except BaseException as caught:
        record.update(complete=False, compiler_validation_complete=False,
                      compiler_validation_error=repr(caught))
        raise
    finally:
        write(path, record)


def validate(record, sinks):
    from generator_evaluation_source_admission_5_0_6 import current_input_matches, registered_epoch_matches
    prerequisites()
    engine.validate(record, sinks)
    require(registered_epoch_matches(Path(__file__), sha(record["adapter_source"].encode()))
            and record["adapter_sha256"] == sha(record["adapter_source"].encode())
            and registered_epoch_matches(Path(engine.__file__), record["execution_engine_sha256"])
            and registered_epoch_matches(WORKER, sha(record["worker_source"].encode()))
            and record["compiler_validation_complete"] is True, "exact adapter, engine, worker and completed admission")
    require(record["preparation_seed_sha256"] == SEED_SHA
            and record["compiler_directory_removed"] is True
            and not Path(record["compiler_directory"]).exists()
            and record["service_jobs_remaining"] == record["compiler_jobs_remaining"] == [],
            "fresh preparation and complete owned-output cleanup")
    require([step["label"] for step in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    for step in record["preparation_steps"]), "both actual current Java compilations")
    prepared = {name for step in record["preparation_steps"] for name in step["command"] if name.endswith(".java")}
    require(set(record["prepared_java_sources"]) == prepared
            and all(current_input_matches(Path(name), value)
                for name, value in record["prepared_java_sources"].items()), "prepared Java source identities")
    capabilities, job = record["capabilities"], record["compiler_job"]
    require(capabilities["schema"] == "teaql.rust_configuration.job.v2"
            and capabilities["artifacts"] == record["artifacts_after"] == job["artifacts"]
            and job["capabilities_sha256"] == sha(json.dumps(capabilities, sort_keys=True).encode())
            and job["worker_sha256"] == sha(record["worker_source"].encode()), "full metadata and immutable compiled Java artifacts")
    require(len(record["process_adaptations"]) == len(record["steps"]) == 41,
            "every existing process has its actual launcher")
    replacements = []
    for step, actual in zip(record["steps"], record["process_adaptations"], strict=True):
        source = list(map(str, step["command"]))
        require(source == actual["source_command"], "source command bound to actual invocation")
        expected = list(source)
        if expected[0] == "mvn":
            expected[0] = maven_command()
        if MAIN in source:
            replacements.append(actual)
            require(step["label"] == "cpp_generate", "only C++ generation uses the new route")
            command = capabilities["cpp_generator"]["command"]
            expected = [source[0], "-cp", command[command.index("-cp") + 1], "CppCompilerServiceProbe",
                        str(Path(record["compiler_directory"]) / "model.json"), source[-1],
                        str(Path(sys.executable).resolve()), str(WORKER),
                        str(Path(record["compiler_directory"]) / "capabilities.json"),
                        str(Path(record["compiler_directory"]) / "service_jobs"),
                        str(Path(record["compiler_directory"]) / "compiler_report.json.gz")]
            if record["controller"]:
                expected.append("--controller")
        require(actual["executed_command"] == expected, "only the declared source-service launcher replacement")
        require(set(actual["environment_changes"]) <= {"CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"},
                "only the locked native Rust linker is normalized")
        for change in actual["environment_changes"].values():
            require(Path(change["after"]).is_file()
                    and sha(Path(change["after"]).read_bytes()) == record["native_compiler"]["sha256"],
                    "actual locked native linker")
    entry = record.get("entry", "controller" if record["controller"] else "service")
    require(entry in ("service", "controller")
            and len(replacements) == 1 and record["controller"] is (entry == "controller"),
            "one actual selected generation entry")
    service = record["compiler_service_process"]
    generated = next(step for step in record["steps"] if step["label"] == "cpp_generate")
    require(service["command"] == replacements[0]["executed_command"]
            and type(service["exit_code"]) is int and service["exit_code"] == generated["exit_code"] == 0
            and service["stdout"] == generated["stdout"] and service["stderr"] == generated["stderr"],
            "actual source-service process result")
    package = record["source_package"]
    entrance = "main.xml" if record["controller"] else "inheritance_graph.xml"
    require(package == dict(schema="teaql.model_source_package.v1", entrance_file=entrance,
                            files={entrance: base64.b64encode(record["model_source"].encode()).decode()}),
            "complete unchanged business model source package")
    require(job["complete"] is True and job["outcome"] == "resolved"
            and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
            and job["original_sources_before"] == job["original_sources_after"], "actual successful immutable Rust compiler job")
    worker = worker_module()
    require(job["references_before"] == job["references_after"] == {
        "reference_generator": worker.GENERATOR, "runtime_reference": worker.RUNTIME}, "locked compiler references")
    raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
    require(json.loads(raw) == package and sha(raw) == job["model_sha256"], "compiler model input binding")
    worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
    document = job["resolution_document"]
    projection = document["projection_resolution"]
    require(document["schema"] == "teaql.rust_configuration.model_generation.v1"
            and engine.same(document["entities"], job["configuration"])
            and document["configuration_observer_sha256"] == sha(job["configuration_observer_source"].encode())
            and document["configuration_artifacts_sha256"] == worker.manifest_sha(job["configuration_artifacts"])
            and job["configuration_artifacts"] == job["observer_artifacts"], "full metadata and projection share the compiled library")
    require(projection["model_sha256"] == job["model_sha256"]
            and projection["compiler_lock_sha256"] == sha(LOCK.read_bytes())
            and projection["source_manifest"] == job["original_sources_before"]
            and projection["mir_sha256"] == job["mir_sha256"] == sha(job["mir"].encode())
            and projection["dependency_lock_sha256"] == sha(job["dependency_lock"].encode())
            and projection["compiler_environment_sha256"] == job["compiler_environment_sha256"],
            "source, MIR, dependency, environment and toolchain binding")
    require(record["cpp_generated_hashes"] == job["cpp_sources"]
            and record["cpp_provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode()),
            "the SDK compiled for business execution is the actual compiler result")
    steps = {step["label"]: step for step in job["steps"]}
    require(all(type(steps[name]["exit_code"]) is int and steps[name]["exit_code"] == 0 for name in (
        "original_compile", "observer_compile", "original_descriptors", "configuration_observer_compile",
        "original_configuration", "cpp_generate")), "actual compiler, metadata observers and emitter")
    require(engine.same(json.loads(steps["original_configuration"]["stdout"]), job["configuration"]),
            "configuration comes from original executable stdout")
    school = next(entity for entity in job["configuration"] if entity["name"] == "School")
    require(school["masks"] == ["name"] and school["limit"] == record["case"]["limit"],
            "compiled audit configuration reaches the governed operation")


def controls(record, sinks):
    validate(record, sinks)
    result = engine.controls(record, sinks)
    changes = (
        ("wrong_adapter", lambda r: r.update(adapter_source="pass\n")),
        ("unfinished_admission", lambda r: r.update(compiler_validation_complete=False)),
        ("missing_preparation", lambda r: r["preparation_steps"].pop()),
        ("missing_java_source", lambda r: r["prepared_java_sources"].pop(next(iter(r["prepared_java_sources"])))),
        ("wrong_capabilities", lambda r: r["compiler_job"].update(capabilities_sha256="0" * 64)),
        ("missing_actual_launcher", lambda r: r["process_adaptations"].pop()),
        ("wrong_actual_launcher", lambda r: r["process_adaptations"][0].update(executed_command=["unqualified"])),
        ("projection_only_capability", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v1")),
        ("changed_compiled_artifact", lambda r: r["artifacts_after"].update(invented="0" * 64)),
        ("wrong_source_package", lambda r: r["source_package"].update(entrance_file="wrong.xml")),
        ("wrong_service_exit", lambda r: r["compiler_service_process"].update(exit_code=False)),
        ("wrong_entry", lambda r: r.update(entry="unknown")),
        ("wrong_controller_route", lambda r: r.update(controller=not r["controller"])),
        ("stale_compiler_output", lambda r: r["compiler_job"].update(outcome="compiler_refusal")),
        ("missing_compiled_property", lambda r: r["compiler_job"]["configuration"][0]["properties"].pop()),
        ("wrong_library", lambda r: r["compiler_job"]["configuration_artifacts"].update(invented="0" * 64)),
        ("wrong_mir", lambda r: r["compiler_job"].update(mir_sha256="0" * 64)),
        ("wrong_sdk", lambda r: r["compiler_job"]["cpp_sources"].update(invented="0" * 64)),
        ("unreaped_worker", lambda r: r["compiler_jobs_remaining"].append("live")),
        ("missing_real_compile", lambda r: r["compiler_job"]["steps"].pop(next(
            i for i, step in enumerate(r["compiler_job"]["steps"]) if step["label"] == "original_compile"))),
    )
    for name, mutate in changes:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed, sinks)
        except (RuntimeError, KeyError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiled governed corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--case", choices=("complete", "partial"), required=True)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--sink-record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    parser.add_argument("--entry", choices=("service", "controller"), default="service")
    args = parser.parse_args()
    if args.execute:
        execute(args.case, args.record.resolve(), args.sink_record.resolve(), args.entry)
    else:
        record, sinks = read(args.record), read(args.sink_record)
        require(record["case"]["name"] == args.case, "selected execution receipt")
        rejected = controls(record, sinks)
        if args.controls:
            require(not args.controls.exists(), "preserve corruption receipts")
            write(args.controls, dict(complete=True, whole_goal_complete=False,
                                      optimized=sys.flags.optimize, controls=rejected,
                                      execution_sha256=sha(args.record.read_bytes()),
                                      sinks_sha256=sha(args.sink_record.read_bytes())))
        print("Verified compiled governed execution:", len(rejected), "controls; optimized=", sys.flags.optimize)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
