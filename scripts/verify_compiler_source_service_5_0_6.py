#!/usr/bin/env python3
"""Execute compiler generation through the source service and existing controller."""

import argparse
import base64
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned
from verify_cpp_compiler_job_5_0_6 import configured_model, cpp_observer
from verify_compiler_projection_generation_5_0_6 import native_command

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
JAVA = ROOT / "docs/parity/compiler_source_service_java_5_0_6.json.gz"
LOCK = ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"


def require(value, message):
    if not value:
        raise RuntimeError("Compiler source service: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def artifact_sha(path):
    if path.is_dir():
        values = {str(p.relative_to(path)): sha(p.read_bytes()) for p in sorted(path.rglob("*")) if p.is_file()}
        return sha("".join(name + "\0" + values[name] + "\n" for name in sorted(values, key=lambda p: p.encode())).encode())
    return sha(path.read_bytes())


def model_package(include):
    text = configured_model(include)
    gateway = '  <gateway name="string()"/>\n'
    children = '  <calibrated_sensor _parent="sensor" calibration_code="string()"/>\n  <sample sensor="sensor()" note="string()"/>\n'
    require(text.count(gateway) == 1 and text.count(children) == 1, "known semantic fixture decomposition")
    text = text.replace(gateway, '  <_include file="entities/gateway.xml"/>\n')
    text = text.replace(children, '  <_include file="../common/readings.xml"/>\n')
    values = {
        "models/main.xml": text.encode(),
        "models/entities/gateway.xml": ('<root>\n' + gateway + '</root>\n').encode(),
        "common/readings.xml": ('<root>\n' + children + '</root>\n').encode(),
        "data/binary.bin": bytes((0, 255, 7, 3)),
    }
    return dict(schema="teaql.model_source_package.v1", entrance_file="models/main.xml",
                files={name: base64.b64encode(data).decode() for name, data in values.items()})


def execute(path):
    require(not path.exists(), "preserve historical receipts")
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  input_hashes_before=capture_inputs(), references_before=references(), steps=[], cases=[])

    def run(label, command, *, env=None, cwd=ROOT, expected=0):
        command = list(map(str, command))
        result = run_owned(command, cwd=cwd, env=env, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, cwd=str(cwd), exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        if expected is not None:
            require(result.returncode == expected, "unexpected process result: " + label)
        return result

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_source_service_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                seed = read(JAVA)
                old_classes = next(s["command"] for s in seed["steps"] if s["label"] == "cpp_javac")
                old_directory = str(Path(old_classes[old_classes.index("-d") + 1]).parent)
                for name in ("original_classes", "cpp_classes"):
                    (owned / name).mkdir()
                for label in ("original_bridge_javac", "cpp_javac"):
                    previous = next(s for s in seed["steps"] if s["label"] == label)
                    command = [item.replace(old_directory, directory) for item in previous["command"]]
                    if label == "cpp_javac":
                        command.append(str(PRODUCER / "generator/src/test/java/com/skynet/codegenerator/SourceGenerationHttpFixture.java"))
                    run(label, command)
                junit = next(s for s in seed["steps"] if s["label"] == "cpp_junit")
                command = [item.replace(old_directory, directory) for item in junit["command"]]
                outcome = run("cpp_junit", command, cwd=PRODUCER / "generator")
                found = re.search(r"OK \((\d+) tests\)", outcome.stdout)
                require(found is not None, "actual Java contract result")
                record["java_test_count"] = int(found[1])
                cpp_cp = command[command.index("-cp") + 1]
                original_test = next(s["command"] for s in seed["steps"] if s["label"] == "original_source_package_contracts")
                original_cp = original_test[original_test.index("-cp") + 1].replace(old_directory, directory)
                run("original_source_package_contracts", [original_test[0], "-cp", original_cp, "org.junit.runner.JUnitCore", "CppModelSourceTest"])
                java = str(Path(__import__("shutil").which("java")).resolve())
                reference_command = [java, "-cp", original_cp, "CppReferenceModel"]
                cpp_command = [java, "-cp", cpp_cp, "CppCompiledModel"]
                artifacts = {str(p): artifact_sha(p) for p in sorted({Path(java), *map(Path, original_cp.split(os.pathsep)),
                                                                      *map(Path, cpp_cp.split(os.pathsep))})}
                temp = owned / "model_\U0001f9ea"
                temp.mkdir()
                jobs = owned / "service_jobs"
                jobs.mkdir()
                linker = str(Path(os.environ["CC"]).resolve())
                config = dict(schema="teaql.rust_configuration.job.v1", reference_generator=dict(
                    repository=str(ROOT / ".reference/teaql_code_gen"), command=reference_command),
                    runtime_reference=dict(repository=str(ROOT / ".reference/teaql-rs")),
                    cpp_generator=dict(command=cpp_command), artifacts=artifacts,
                    temporary_root=str(temp), timeout_seconds=600,
                    toolchain_lock=dict(path=str(LOCK), sha256=sha(LOCK.read_bytes())),
                    protocol=dict(path=str(ROOT / "scripts/rust_configuration_protocol.py"),
                                  sha256=sha((ROOT / "scripts/rust_configuration_protocol.py").read_bytes())),
                    linker=dict(path=linker, sha256=sha(Path(linker).read_bytes())))
                capabilities = owned / "capabilities.json"
                capabilities.write_text(json.dumps(config, sort_keys=True))
                record.update(capabilities=config, worker_source=WORKER.read_text(),
                              java_sources={name: sha(Path(name).read_bytes()) for name in set(seed["java_sources"]) | {
                                  str(PRODUCER / "generator/src/test/java/com/skynet/codegenerator/SourceGenerationHttpFixture.java")}})
                include = temp / "external_count.rs"
                include.write_text("2usize\n")
                complete = model_package(include)
                empty = dict(schema="teaql.model_source_package.v1", entrance_file="models/main.xml", files={
                    "models/main.xml": base64.b64encode(b'<root name="edge_device" org="teaql" data_service="sqlite"><gateway name="string()"/></root>\n').decode()})
                scenarios = (("service_multi", complete, "2usize\n", False, 0, "edge_compiler_router"),
                             ("service_refusal", complete, "true\n", False, 2, None),
                             ("service_zero", empty, "2usize\n", False, 0, "edge_device"),
                             ("controller_zip", complete, "2usize\n", True, 0, "edge_compiler_router"))
                for name, source, included, controller, code, namespace in scenarios:
                    include.write_text(included)
                    packaged = temp / (name + ".json")
                    packaged.write_text(json.dumps(source, sort_keys=True))
                    output = owned / (name + "_cpp")
                    report = owned / (name + ".json.gz")
                    env = dict(os.environ, TEAQL_CPP_COMPILER_JOB_VALUE="allowed", PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
                    command = [java, "-cp", cpp_cp, "CppCompilerServiceProbe", packaged, output, Path(sys.executable).resolve(),
                               WORKER, capabilities, jobs, report, *(["--controller"] if controller else [])]
                    outcome = run(name, command, env=env, expected=None)
                    job = read(report) if report.is_file() else None
                    row = dict(name=name, source_package=source, included_source=included, job=job, controller=controller,
                               cli_exit_code=outcome.returncode, sdk_exists=output.exists(), service_jobs_remaining=list(map(str, jobs.iterdir())))
                    record["cases"].append(row)
                    write(path, record)
                    require(outcome.returncode == code and job is not None, "actual configured service outcome")
                    require(not list(jobs.iterdir()), "all service-owned jobs must close before return/report delivery")
                    if code:
                        require(not output.exists() and job["outcome"] == "compiler_refusal", "no silent finite fallback or SDK")
                        continue
                    observer = owned / (name + ".cpp")
                    observer.write_text(cpp_observer(namespace, [item["type"] for item in job["original_inventory"]["entities"]]))
                    executable = owned / (name + "_native")
                    run(name + "_native_compile", native_command(output / "include", observer, executable))
                    actual = run(name + "_native_execute", [executable], env=env)
                    row.update(cpp_observer_source=observer.read_text(), cpp_descriptors=json.loads(actual.stdout),
                               returned_sdk={str(p.relative_to(output)): sha(p.read_bytes()) for p in output.rglob("*") if p.is_file()})
                    expected = [{key: value for key, value in item.items() if key not in ("module", "type")} for item in job["descriptors"]]
                    require(row["cpp_descriptors"] == expected, "actual source-service generated C++ descriptors")
                    write(path, record)
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(owned_directory_removed=not Path(record["owned_directory"]).exists(),
                      input_hashes_after=capture_inputs(), references_after=references())
        require(record["input_hashes_before"] == record["input_hashes_after"]
                and record["references_before"] == record["references_after"], "frozen source frame")
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
    from compiler_model_configuration_source_admission_5_0_6 import current_input_matches as model_input_matches

    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["owned_directory_removed"] is True and record["java_test_count"] == 252,
            "bounded service qualification and all Java contracts")
    require(record["input_hashes_before"] == record["input_hashes_after"] and record["references_before"] == record["references_after"],
            "source and reference stability")
    require(model_input_matches(WORKER, sha(record["worker_source"].encode())) and model_input_matches(Path(__file__), sha(record["driver_source"].encode()))
            and all(model_input_matches(Path(name), value) for name, value in record["java_sources"].items()),
            "actual executed implementation inputs")
    require([row["name"] for row in record["cases"]] == ["service_multi", "service_refusal", "service_zero", "controller_zip"],
            "all actual default-service and controller jobs")
    labels = [step["label"] for step in record["steps"]]
    expected = ["original_bridge_javac", "cpp_javac", "cpp_junit", "original_source_package_contracts"]
    for name in ("service_multi", "service_refusal", "service_zero", "controller_zip"):
        expected.append(name)
        if name != "service_refusal":
            expected += [name + "_native_compile", name + "_native_execute"]
    require(labels == expected and all(type(step["exit_code"]) is int and step["exit_code"] == (2 if step["label"] == "service_refusal" else 0)
                                      for step in record["steps"]), "all actual terminal tool/Java/native steps")
    for row in record["cases"]:
        job = row["job"]
        require(job["complete"] is True and job["source_format"] == "source_package"
                and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and row["service_jobs_remaining"] == []
                and job["original_sources_before"] == job["original_sources_after"], "source job completion and cleanup")
        actual = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(sha(actual) == job["model_sha256"] and json.loads(actual) == row["source_package"],
                "exact source entrance, filenames and complete file bytes")
        compiles = [s for s in job["steps"] if s["label"] == "original_compile"]
        require(len(compiles) == 1 and type(compiles[0]["exit_code"]) is int, "one actual original model compile")
        if row["name"] == "service_refusal":
            require(row["cli_exit_code"] == 2 and row["sdk_exists"] is False
                    and job["outcome"] == "compiler_refusal" and compiles[0]["exit_code"] == 101
                    and any(error["code"] == "E0308" for error in job["compiler_errors"])
                    and all(s["label"] != "cpp_generate" for s in job["steps"]), "actual refusal without fallback")
        else:
            require(row["cli_exit_code"] == 0 and row["sdk_exists"] is True and compiles[0]["exit_code"] == 0
                    and job["outcome"] == "resolved" and row["returned_sdk"] == job["cpp_sources"]
                    and len(job["observer_artifacts"]) == 3, "same-artifact compilation and complete SDK transfer")
            require(row["cpp_descriptors"] == [{key: value for key, value in d.items() if key not in ("module", "type")}
                                               for d in job["descriptors"]], "actual original/native descriptors")
    require(record["cases"][-1]["controller"] is True
            and len(record["cases"][-1]["source_package"]["files"]) == 4, "real controller ZIP route with all source files")
    require(record["cases"][2]["job"]["protocol"]["projections"] == [], "zero-projection service model")


def controls(path):
    record = read(path)
    validate(record)
    changes = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_controller", lambda r: r["cases"].pop()),
        ("missing_native_compile", lambda r: r["steps"].pop(5)),
        ("invented_native_success", lambda r: r["steps"][5].update(exit_code=True)),
        ("lost_source_file", lambda r: r["cases"][0]["source_package"]["files"].pop("data/binary.bin")),
        ("wrong_entrance", lambda r: r["cases"][0]["source_package"].update(entrance_file="main.xml")),
        ("wrong_package_binding", lambda r: r["cases"][0]["job"].update(model_sha256="0" * 64)),
        ("incomplete_sdk", lambda r: r["cases"][0]["returned_sdk"].pop("RUST_CONFIGURATION.json")),
        ("wrong_descriptor", lambda r: r["cases"][0]["cpp_descriptors"][0].update(owner="wrong")),
        ("silent_fallback", lambda r: r["cases"][1].update(sdk_exists=True)),
        ("wrong_refusal", lambda r: r["cases"][1]["job"]["compiler_errors"][0].update(code="wrong")),
        ("unclosed_service_job", lambda r: r["cases"][0]["service_jobs_remaining"].append("active")),
        ("mutated_original", lambda r: r["cases"][0]["job"]["original_sources_after"].update(invented="0" * 64)),
        ("dropped_java_contract", lambda r: r.update(java_test_count=251)),
    )
    result = []
    for name, mutate in changes:
        modified = copy.deepcopy(record)
        mutate(modified)
        try:
            validate(modified)
        except RuntimeError as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Unexpected service receipt acceptance: " + name)
    return dict(complete=True, optimized=sys.flags.optimize, execution_sha256=sha(path.read_bytes()),
                controls=result, valid_record_accepted=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path)
    parser.add_argument("--controls", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.execute)
    elif args.controls and args.output:
        require(not args.output.exists(), "preserve prior controls")
        result = controls(args.controls)
        write(args.output, result)
        print(json.dumps(dict(complete=True, optimized=result["optimized"], controls=len(result["controls"]))))
    else:
        parser.error("Choose --execute or --controls with --output")


if __name__ == "__main__":
    main()
