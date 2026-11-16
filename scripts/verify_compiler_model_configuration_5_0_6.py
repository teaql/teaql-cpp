#!/usr/bin/env python3
"""Qualify complete compiled model metadata through the source service/controller."""

import argparse
import base64
import copy
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import xml.etree.ElementTree as ET

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned
from verify_compiler_projection_generation_5_0_6 import native_command
from verify_compiler_source_service_5_0_6 import artifact_sha, model_package

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
SEED = ROOT / "docs/parity/compiler_source_service_current_5_0_6.json.gz"
LOCK = ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"
OBSERVER = ROOT / "tests/parity/compiler_model_configuration_probe.cpp"
MODEL_SCHEMA = "teaql.rust_configuration.model_generation.v1"
CAPABILITIES_SCHEMA = "teaql.rust_configuration.job.v2"
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same


def require(value, message):
    if not value:
        raise RuntimeError("Compiler model configuration: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def worker_module():
    spec = importlib.util.spec_from_file_location("teaql_model_configuration_worker", WORKER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def source_package(entrance, files):
    return dict(schema="teaql.model_source_package.v1", entrance_file=entrance,
                files={name: base64.b64encode(content).decode() for name, content in files.items()})


def configured_sources(include, customized=False):
    source = model_package(include)
    if customized:
        name = "models/main.xml"
        tree = ET.fromstring(base64.b64decode(source["files"][name]))
        sensor = tree.find("sensor")
        require(sensor is not None, "actual configured sensor fixture")
        sensor.set("_audit_value_max_len", sensor.attrib["_audit_value_max_len"]
                   + ', table="compiled_sensor_data", audit_mask_fields="name", audit_value_max_len_int=21')
        source["files"][name] = base64.b64encode(ET.tostring(tree)).decode()
    return source


def scalar_sources():
    original = (ROOT / "tests/generator/optional_types.xml").read_bytes()
    tree = ET.fromstring(original)
    reading = tree.find("reading")
    require(reading is not None and reading.attrib["active"] == "bool()?"
            and reading.attrib["recorded_at"] == "dateTime()?", "exact retained optional scalar fixture")
    tree.insert(0, ET.Element("gateway", {"name": "string()"}))
    reading.set("gateway", "gateway(contextRoot=true)")
    reading.set("recorded_at", "dateTime()")
    derivation = dict(source="tests/generator/optional_types.xml", source_sha256=sha(original), edits={
        "root.gateway": [None, '<gateway name="string()"/>'],
        "reading.gateway": [None, "gateway(contextRoot=true)"],
        "reading.recorded_at": ["dateTime()?", "dateTime()"]})
    return original, ET.tostring(tree), derivation


def cpp_observer(namespace, entities):
    return ("#include <" + namespace + ".hpp>\n" + OBSERVER.read_text()
            + "\nusing namespace " + namespace + ";\nint main() {\n"
            "    std::pmr::set_default_resource(std::pmr::null_memory_resource());\n"
            "    GlobalContext global;\n    auto user_context = global.make_user_context();\n"
            "    {\n        auto resource = user_context->memory_resource();\n        Json::Array values(resource);\n"
            + "".join('        values.emplace_back(describe_configuration<' + row["type"] + '>(resource, '
                      + json.dumps(row["module"]) + ', ' + json.dumps(row["type"]) + '));\n' for row in entities)
            + "        std::cout << Json(resource, std::move(values)).dump(resource) << '\\n';\n    }\n"
            "    user_context.reset();\n    if (global.live_user_bytes() != 0) {\n"
            '        throw std::runtime_error("Compiled model configuration retained Context memory");\n    }\n}\n')


def expected_configuration(name, configuration):
    expected = copy.deepcopy(configuration)
    if name == "service_scalars":
        payload = next(property for entity in expected for property in entity["properties"] if property["name"] == "payload")
        require(payload["data_type"] == "LargeText", "actual approved Rust JSON metadata defect")
        payload["data_type"] = "Json"
    return expected


def execute(path):
    require(not path.exists(), "preserve execution evidence")
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  observer_template=OBSERVER.read_text(), worker_source=WORKER.read_text(),
                  input_hashes_before=capture_inputs(), references_before=references(), steps=[], cases=[])

    def run(label, command, *, cwd=ROOT, env=None, expected=0):
        command = list(map(str, command))
        result = run_owned(command, cwd=cwd, env=env, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, cwd=str(cwd), exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        if expected is not None:
            require(result.returncode == expected, "unexpected terminal result: " + label)
        return result

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_model_configuration_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                seed = read(SEED)
                previous = next(step for step in seed["steps"] if step["label"] == "cpp_javac")["command"]
                old_directory = str(Path(previous[previous.index("-d") + 1]).parent)
                for name in ("cpp_classes", "original_classes"):
                    (owned / name).mkdir()
                extra = [PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp" / name
                         for name in ("CppModelConfiguration.java", "CppCompilerModelConfiguration.java")]
                extra.append(PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppCompilerModelConfigurationTest.java")
                java_sources = set(seed["java_sources"])
                java_sources.update(map(str, extra))
                for label in ("original_bridge_javac", "cpp_javac"):
                    command = [part.replace(old_directory, directory) for part in
                               next(step for step in seed["steps"] if step["label"] == label)["command"]]
                    if label == "cpp_javac":
                        command += list(map(str, extra))
                    run(label, command)
                command = [part.replace(old_directory, directory) for part in
                           next(step for step in seed["steps"] if step["label"] == "cpp_junit")["command"]]
                command.append("com.skynet.codegenerator.cpp.CppCompilerModelConfigurationTest")
                outcome = run("cpp_junit", command, cwd=PRODUCER / "generator")
                count = re.search(r"OK \((\d+) tests\)", outcome.stdout)
                require(count is not None, "actual complete Java test result")
                record["java_test_count"] = int(count[1])
                cpp_cp = command[command.index("-cp") + 1]
                command = [part.replace(old_directory, directory) for part in
                           next(step for step in seed["steps"] if step["label"] == "original_source_package_contracts")["command"]]
                run("original_source_package_contracts", command)
                original_cp = command[command.index("-cp") + 1]
                for mode in (0, 1):
                    run("worker_units_" + str(mode), [sys.executable, *(["-O"] if mode else []),
                        PRODUCER / "scripts/test_cpp_rust_compiler_job.py"], env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"))
                java = str(Path(shutil.which("java")).resolve())
                artifacts = {str(p): artifact_sha(p) for p in sorted({Path(java), *map(Path, original_cp.split(os.pathsep)),
                                                                      *map(Path, cpp_cp.split(os.pathsep))})}
                temporary = owned / "model_\U0001f9ea"
                temporary.mkdir()
                jobs = owned / "service_jobs"
                jobs.mkdir()
                linker = str(Path(os.environ["CC"]).resolve())
                config = dict(schema=CAPABILITIES_SCHEMA,
                              reference_generator=dict(repository=str(ROOT / ".reference/teaql_code_gen"), command=[java, "-cp", original_cp, "CppReferenceModel"]),
                              runtime_reference=dict(repository=str(ROOT / ".reference/teaql-rs")),
                              cpp_generator=dict(command=[java, "-cp", cpp_cp, "CppCompiledModel"]), artifacts=artifacts,
                              temporary_root=str(temporary), timeout_seconds=600,
                              toolchain_lock=dict(path=str(LOCK), sha256=sha(LOCK.read_bytes())),
                              protocol=dict(path=str(ROOT / "scripts/rust_configuration_protocol.py"),
                                            sha256=sha((ROOT / "scripts/rust_configuration_protocol.py").read_bytes())),
                              linker=dict(path=linker, sha256=sha(Path(linker).read_bytes())))
                capabilities = owned / "capabilities.json"
                capabilities.write_text(json.dumps(config, sort_keys=True))
                record.update(capabilities=config, java_sources={name: sha(Path(name).read_bytes()) for name in java_sources})
                include = temporary / "external_count.rs"
                include.write_text("2usize\n")
                scalar_source, compiled_scalars, derivation = scalar_sources()
                original_scalars = source_package("models/optional_types.xml", {"models/optional_types.xml": scalar_source})
                scalars = source_package("models/compiled_scalars.xml", {"models/compiled_scalars.xml": compiled_scalars})
                record["scalar_fixture_derivation"] = derivation
                cases = (("service_full", configured_sources(include), "2usize\n", False, 0, "edge_compiler_router"),
                         ("service_refusal", configured_sources(include), "true\n", False, 2, None),
                         ("optional_source_refusal", original_scalars, "2usize\n", False, 2, None),
                         ("service_scalars", scalars, "2usize\n", False, 0, "cpp_optional_types"),
                         ("controller_full", configured_sources(include, True), "2usize\n", True, 0, "edge_compiler_router"))
                for name, source, included, controller, code, namespace in cases:
                    include.write_text(included)
                    packaged = temporary / (name + ".json")
                    packaged.write_text(json.dumps(source, sort_keys=True))
                    output, report = owned / (name + "_cpp"), owned / (name + ".json.gz")
                    env = dict(os.environ, TEAQL_CPP_COMPILER_JOB_VALUE="allowed", PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
                    outcome = run(name, [java, "-cp", cpp_cp, "CppCompilerServiceProbe", packaged, output,
                                        Path(sys.executable).resolve(), WORKER, capabilities, jobs, report,
                                        *(["--controller"] if controller else [])], env=env, expected=None)
                    job = read(report) if report.exists() else None
                    row = dict(name=name, source_package=source, included_source=included, controller=controller,
                               cli_exit_code=outcome.returncode, job=job, sdk_exists=output.exists(), service_jobs_remaining=list(map(str, jobs.iterdir())))
                    record["cases"].append(row)
                    write(path, record)
                    require(outcome.returncode == code and job is not None and not list(jobs.iterdir()), "actual completed source-service compiler outcome")
                    if code:
                        require(not output.exists() and job["outcome"] == "compiler_refusal", "no metadata fallback after reference refusal")
                        continue
                    observer = owned / (name + ".cpp")
                    observer.write_text(cpp_observer(namespace, job["original_inventory"]["entities"]))
                    executable = owned / (name + "_native")
                    run(name + "_native_compile", native_command(output / "include", observer, executable))
                    actual = run(name + "_native_execute", [executable], env=env)
                    row.update(cpp_observer_source=observer.read_text(), cpp_configuration=json.loads(actual.stdout),
                               cpp_provenance=json.loads((output / "RUST_CONFIGURATION.json").read_bytes()),
                               cpp_sources={str(p.relative_to(output)): sha(p.read_bytes()) for p in output.rglob("*") if p.is_file()})
                    require(same(row["cpp_configuration"], expected_configuration(name, job["configuration"])), "full original/C++ compiled configuration differs")
                    write(path, record)
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(owned_directory_removed=not Path(directory).exists(), input_hashes_after=capture_inputs(), references_after=references())
        require(record["input_hashes_before"] == record["input_hashes_after"] and record["references_before"] == record["references_after"], "frozen execution inputs")
        record["complete"] = True
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    from generator_evaluation_source_admission_5_0_6 import current_input_matches, registered_epoch_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False and record["java_test_count"] == 256
            and record["owned_directory_removed"] is True, "complete bounded metadata qualification")
    require(registered_epoch_matches(Path(__file__), sha(record["driver_source"].encode())) and record["observer_template"] == OBSERVER.read_text()
            and record["worker_source"] == WORKER.read_text(), "actual execution source identities")
    require(record["input_hashes_before"] == record["input_hashes_after"] and record["references_before"] == record["references_after"], "stable source and reference frame")
    worker = worker_module()
    require(record["references_after"] == {"teaql-rs": worker.RUNTIME, "teaql_code_gen": worker.GENERATOR}
            and record["capabilities"]["schema"] == CAPABILITIES_SCHEMA, "locked baseline and explicit full-metadata capability version")
    require(all(current_input_matches(Path(name), value) for name, value in record["java_sources"].items()), "actual qualified compiled Java source")
    require([row["name"] for row in record["cases"]] == ["service_full", "service_refusal", "optional_source_refusal", "service_scalars", "controller_full"], "complete service/controller matrix")
    scalar_source, compiled_scalars, derivation = scalar_sources()
    require(record["scalar_fixture_derivation"] == derivation, "explicit valid-scalar fixture derivation")
    require(base64.b64decode(record["cases"][3]["source_package"]["files"]["models/compiled_scalars.xml"])
            == compiled_scalars, "actual separate valid scalar fixture")
    labels = ["original_bridge_javac", "cpp_javac", "cpp_junit", "original_source_package_contracts", "worker_units_0", "worker_units_1"]
    for row in record["cases"]:
        labels.append(row["name"])
        job = row["job"]
        require(job["complete"] is True and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and row["service_jobs_remaining"] == [] and job["original_sources_before"] == job["original_sources_after"], "completed immutable original job and cleanup")
        require(job["references_before"] == job["references_after"] == {
            "reference_generator": worker.GENERATOR, "runtime_reference": worker.RUNTIME}, "actual job reference identities")
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(json.loads(raw) == row["source_package"] and sha(raw) == job["model_sha256"], "complete actual source package binding")
        if row["name"] in ("service_refusal", "optional_source_refusal"):
            require(row["cli_exit_code"] == 2 and row["sdk_exists"] is False and job["outcome"] == "compiler_refusal"
                    and any(error["code"] == "E0308" for error in job["compiler_errors"])
                    and "configuration" not in job, "actual compiler refusal without stale metadata")
            if row["name"] == "optional_source_refusal":
                require({error["code"] for error in job["compiler_errors"]} == {"E0308", "E0599"}
                        and base64.b64decode(row["source_package"]["files"]["models/optional_types.xml"])
                        == (ROOT / "tests/generator/optional_types.xml").read_bytes(), "unchanged original fixture and actual reference refusal")
            continue
        labels += [row["name"] + "_native_compile", row["name"] + "_native_execute"]
        require(row["cli_exit_code"] == 0 and row["sdk_exists"] is True and job["outcome"] == "resolved", "actual successful compiled configuration")
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        document = job["resolution_document"]
        projection = document["projection_resolution"]
        require(projection["model_sha256"] == job["model_sha256"]
                and projection["generator_commit"] == worker.GENERATOR and projection["runtime_commit"] == worker.RUNTIME
                and projection["target"] == worker.TARGET
                and projection["compiler_lock_sha256"] == sha(LOCK.read_bytes())
                and projection["source_manifest"] == job["original_sources_before"]
                and projection["mir_sha256"] == job["mir_sha256"] == sha(job["mir"].encode())
                and projection["dependency_lock_sha256"] == sha(job["dependency_lock"].encode())
                and projection["compiler_environment_sha256"] == job["compiler_environment_sha256"], "compiled source, dependency and environment bindings")
        require(document["schema"] == MODEL_SCHEMA and same(document["entities"], job["configuration"])
                and document["configuration_observer_sha256"] == sha(job["configuration_observer_source"].encode())
                and document["configuration_artifacts_sha256"] == worker.manifest_sha(job["configuration_artifacts"])
                and job["configuration_artifacts"] == job["observer_artifacts"], "same-library metadata and projection provenance")
        require(row["cpp_sources"] == job["cpp_sources"] and row["cpp_provenance"]["schema"] == MODEL_SCHEMA
                and row["cpp_provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode())
                and same(row["cpp_configuration"], expected_configuration(row["name"], job["configuration"])), "complete actual native metadata and SDK transfer")
        steps = {step["label"]: step for step in job["steps"]}
        require(all(steps[name]["exit_code"] == 0 for name in ("original_compile", "observer_compile", "original_descriptors", "configuration_observer_compile", "original_configuration", "cpp_generate")), "actual original/compiler/observer/emitter execution")
        require(same(job["configuration"], json.loads(steps["original_configuration"]["stdout"]))
                and job["configuration_observer_source"] == worker.configuration_source(job["original_inventory"]["entities"]), "complete metadata from the actual original observer")
        native_step = next(step for step in record["steps"] if step["label"] == row["name"] + "_native_execute")
        require(same(row["cpp_configuration"], json.loads(native_step["stdout"])), "actual native configuration stdout binding")
    require([step["label"] for step in record["steps"]] == labels
            and all(type(step["exit_code"]) is int and step["exit_code"] == (2 if step["label"] in ("service_refusal", "optional_source_refusal") else 0)
                    for step in record["steps"]), "all actual terminal Java/compiler/native steps")
    steps = {step["label"]: step for step in record["steps"]}
    require("OK (256 tests)" in steps["cpp_junit"]["stdout"]
            and "OK (2 tests)" in steps["original_source_package_contracts"]["stdout"]
            and all("Ran 12 tests" in steps["worker_units_" + str(mode)]["stderr"] for mode in (0, 1)), "raw Java and ordinary/optimized Python test outcomes")
    customized = record["cases"][-1]
    sensor = next(entity for entity in customized["job"]["configuration"] if entity["module"] == "sensor")
    require(customized["controller"] is True and sensor["table"] == "compiled_sensor_data" and sensor["masks"] == ["name"]
            and sensor["limit"] == "21", "compiled table/audit overrides survive controller generation")


def controls(path):
    record = read(path)
    validate(record)
    changes = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_controller", lambda r: r["cases"].pop()),
        ("missing_entity", lambda r: r["cases"][0]["job"]["configuration"].pop()),
        ("changed_table", lambda r: r["cases"][0]["cpp_configuration"][0].update(table="wrong")),
        ("changed_property", lambda r: r["cases"][0]["cpp_configuration"][0]["properties"][0].update(nullable=True)),
        ("coerced_native_flag", lambda r: r["cases"][0]["cpp_configuration"][0]["properties"][0].update(nullable=0)),
        ("wrong_artifact", lambda r: r["cases"][0]["job"]["configuration_artifacts"].update(invented="0" * 64)),
        ("wrong_observer", lambda r: r["cases"][0]["job"].update(configuration_observer_source="invented")),
        ("wrong_metadata_schema", lambda r: r["cases"][0]["job"]["resolution_document"].update(schema="invented")),
        ("wrong_native_provenance", lambda r: r["cases"][0]["cpp_provenance"].update(schema="invented")),
        ("fallback_after_refusal", lambda r: r["cases"][1].update(sdk_exists=True)),
        ("missing_real_compile", lambda r: r["cases"][0]["job"]["steps"].pop(next(i for i, step in enumerate(r["cases"][0]["job"]["steps"]) if step["label"] == "configuration_observer_compile"))),
        ("missing_native_compile", lambda r: r["steps"].pop(7)),
        ("unclosed_job", lambda r: r["cases"][0]["service_jobs_remaining"].append("live")),
        ("lost_java_contract", lambda r: r.update(java_test_count=255)),
        ("hidden_scalar_derivation", lambda r: r["scalar_fixture_derivation"]["edits"].clear()),
        ("wrong_reference", lambda r: r["references_after"].update({"teaql-rs": "wrong"})),
        ("wrong_capability_version", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v1")),
        ("wrong_mir_binding", lambda r: r["cases"][0]["job"]["resolution_document"]["projection_resolution"].update(mir_sha256="0" * 64)),
    )
    rejected = []
    for name, mutate in changes:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (RuntimeError, KeyError) as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Compiled configuration corruption accepted: " + name)
    return dict(complete=True, optimized=sys.flags.optimize, execution_sha256=sha(path.read_bytes()), controls=rejected, valid_record_accepted=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path)
    parser.add_argument("--controls", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.execute)
    elif args.controls and args.output:
        require(not args.output.exists(), "preserve corruption evidence")
        result = controls(args.controls)
        write(args.output, result)
        print(json.dumps(dict(complete=True, optimized=result["optimized"], controls=len(result["controls"]))))
    else:
        parser.error("Choose --execute or --controls with --output")


if __name__ == "__main__":
    main()
