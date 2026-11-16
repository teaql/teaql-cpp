#!/usr/bin/env python3
"""Qualify real model-to-compiler-to-C++ jobs, with no retained compiler-fact input."""

import argparse
import copy
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from xml.sax.saxutils import escape

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned
from verify_compiler_projection_generation_5_0_6 import native_command

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
ORIGINAL = ROOT / ".reference/teaql_code_gen"
RUNTIME = ROOT / ".reference/teaql-rs"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
BRIDGE = PRODUCER / "tools/cpp_reference_model/CppReferenceModel.java"
TOOLCHAIN = ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"
PROTOCOL = ROOT / "scripts/rust_configuration_protocol.py"
BUILD_SEED = ROOT / "docs/parity/ap02_generated_rust_mir_resolution_probe_5_0_6.json.gz"
PREVIOUS = ROOT / "docs/parity/compiler_projection_generation_5_0_6.json.gz"


def require(condition, message):
    if not condition:
        raise RuntimeError("Compiler job qualification: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, record):
    path.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))


def artifact_sha(path):
    if path.is_dir():
        values = {str(p.relative_to(path)): sha(p.read_bytes()) for p in sorted(path.rglob("*")) if p.is_file()}
        return sha("".join(name + "\0" + values[name] + "\n" for name in sorted(values, key=lambda p: p.encode())).encode())
    return sha(path.read_bytes())


def configured_model(include):
    expression = ("{ const fn first() -> usize { 2 } const fn second() -> usize { 3 } "
                  "let selected = if true { first } else { second }; let _ = selected; "
                  'assert!(env!("TEAQL_CPP_COMPILER_JOB_VALUE").as_bytes()[0] == b\'a\'); '
                  "include!(" + json.dumps(str(include), ensure_ascii=False) + ") }")
    attribute = ('13, entity="SensorWire", reverse_relation(name="compiled_samples",target="Sample",'
                 'foreign_key="sensor",many,json_type="<[crate::sample::Sample; '
                 + expression.replace('"', '\\"') + '] as IntoIterator>::Item")')
    return ('<root name="edge_compiler_router" org="teaql" data_service="sqlite" '
            'rust_crate_name="router-capability-core">\n'
            '  <gateway name="string()"/>\n'
            '  <sensor gateway="gateway(contextRoot=true)" name="string()" _audit_value_max_len="'
            + escape(attribute, {'"': "&quot;"}) + '"/>\n'
            '  <calibrated_sensor _parent="sensor" calibration_code="string()"/>\n'
            '  <sample sensor="sensor()" note="string()"/>\n</root>\n')


def cpp_observer(namespace, types):
    prefix = (ROOT / "tests/parity/compiler_projection_metadata_probe.cpp").read_text().split("int main() {", 1)[0]
    prefix = prefix.replace("#include <cpp_inheritance_graph.hpp>", "#include <" + namespace + ".hpp>")
    prefix = prefix.replace("using namespace cpp_inheritance_graph;", "using namespace " + namespace + ";")
    return prefix + """int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    {
        auto resource = user_context->memory_resource();
        Json::Array descriptors(resource);
""" + "".join("        descriptors.emplace_back(describe<" + name + ">(resource));\n" for name in types) + """        std::cout << Json(resource, std::move(descriptors)).dump(resource) << '\\n';
    }
    user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Compiler job SDK retained UserContext memory");
    }
}
"""


def execute(path):
    require(not path.exists(), "preserve previous execution receipts")
    record = dict(complete=False, whole_goal_complete=False, input_hashes_before=capture_inputs(),
                  references_before=references(), steps=[], cases=[], driver_source=Path(__file__).read_text(),
                  worker_source=WORKER.read_text(), bridge_source=BRIDGE.read_text())

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
        with tempfile.TemporaryDirectory(prefix="teaql_cpp_compiler_job_qualification_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                seed, previous = read(BUILD_SEED), read(PREVIOUS)
                command = next(s["command"] for s in seed["steps"] if s["label"] == "original_fresh_java")
                cp = command[command.index("-cp") + 1]
                sources = [Path(p) for p in command if p.endswith(".java") and not p.startswith("/tmp/")]
                original_classes, cpp_classes = owned / "original_classes", owned / "cpp_classes"
                original_classes.mkdir()
                cpp_classes.mkdir()
                run("original_javac", ["javac", "--release", "17", "-cp", cp, "-d", original_classes, *sources, BRIDGE])
                reference_cp = os.pathsep.join([str(original_classes), str(ORIGINAL / "generator/src/main/resources"),
                                               str(ORIGINAL / "model/src/main/resources"), cp])
                cpp_compile = next(s["command"] for s in previous["steps"] if s["label"] == "cpp_javac")
                cpp_deps = cpp_compile[cpp_compile.index("-cp") + 1]
                cpp_sources = [Path(p) for p in previous["java_sources"] if "/src/main/java/" in p]
                require(all(p.is_file() for p in cpp_sources), "all actual current producer sources exist")
                run("cpp_javac", ["javac", "--release", "17", "-cp", cpp_deps, "-d", cpp_classes, *cpp_sources])
                cpp_cp = os.pathsep.join([str(cpp_classes), str(PRODUCER / "generator/src/main/resources"),
                                         str(PRODUCER / "model/src/main/resources"), cpp_deps])
                java = str(Path(shutil.which("java")).resolve())
                reference_command = [java, "-cp", reference_cp, "CppReferenceModel"]
                cpp_command = [java, "-cp", cpp_cp, "com.skynet.codegenerator.cpp.CppCoreGenerator"]
                artifacts = {str(p): artifact_sha(p) for p in sorted({Path(java), *map(Path, reference_cp.split(os.pathsep)),
                                                                      *map(Path, cpp_cp.split(os.pathsep))})}
                linker = str(Path(os.environ["CC"]).resolve())
                model_directory = owned / "model_\U0001f9ea"
                model_directory.mkdir()
                config = dict(schema="teaql.rust_configuration.job.v1",
                              reference_generator=dict(repository=str(ORIGINAL), command=reference_command),
                              runtime_reference=dict(repository=str(RUNTIME)), cpp_generator=dict(command=cpp_command),
                              artifacts=artifacts, temporary_root=str(model_directory), timeout_seconds=600,
                              toolchain_lock=dict(path=str(TOOLCHAIN), sha256=sha(TOOLCHAIN.read_bytes())),
                              protocol=dict(path=str(PROTOCOL), sha256=sha(PROTOCOL.read_bytes())),
                              linker=dict(path=linker, sha256=sha(Path(linker).read_bytes())))
                config_path = owned / "capabilities.json"
                config_path.write_text(json.dumps(config, sort_keys=True))
                record.update(capabilities=config, java_sources={str(p): sha(p.read_bytes())
                                                               for p in {*sources, BRIDGE, *cpp_sources}})
                for optimized in (0, 1):
                    run("unit_controls_" + str(optimized), [sys.executable, *(["-O"] if optimized else []),
                        PRODUCER / "scripts/test_cpp_rust_compiler_job.py"])
                include = model_directory / "external_count.rs"
                include.write_text("2usize\n")
                model = model_directory / "model.xml"
                model.write_text(configured_model(include))
                cases = (("configured", "allowed", "2usize\n", 0, "edge_compiler_router"),
                         ("changed_include", "allowed", "true\n", 2, None),
                         ("changed_environment", "denied", "2usize\n", 2, None),
                         ("zero_relations", "allowed", "2usize\n", 0, "edge_device"))
                for name, value, included, code, namespace in cases:
                    if name == "zero_relations":
                        model.write_text('<root name="edge_device" org="teaql" data_service="sqlite">'
                                         '<gateway name="string()"/></root>\n')
                    include.write_text(included)
                    output, report = owned / (name + "_cpp"), owned / (name + ".json.gz")
                    env = dict(os.environ, TEAQL_CPP_COMPILER_JOB_VALUE=value, TEAQL_LOG_ENDPOINT="off")
                    result = run(name + "_job", [sys.executable, WORKER, model, output, "--capabilities", config_path,
                                                "--report", report], env=env, expected=None)
                    job = read(report) if report.is_file() else None
                    row = dict(name=name, model=model.read_text(), included_source=included,
                               environment_value=value, job=job, cli_exit_code=code)
                    record["cases"].append(row)
                    write(path, record)
                    require(result.returncode == code and job is not None, "expected compiler-job terminal result")
                    require(job["complete"] is True and job["owned_directory_removed"] is True
                            and job["publication_directory_removed"] is True
                            and not Path(job["owned_directory"]).exists(), "terminal worker and owned cleanup")
                    if code:
                        require(job["outcome"] == "compiler_refusal" and not output.exists(), "no SDK on actual compiler refusal")
                        continue
                    require(job["outcome"] == "resolved", "actual successful generation")
                    observer = owned / (name + "_observer.cpp")
                    observer.write_text(cpp_observer(namespace, [entity["type"] for entity in job["original_inventory"]["entities"]]))
                    executable = owned / (name + "_observer")
                    run(name + "_native_compile", native_command(output / "include", observer, executable))
                    observation = run(name + "_native_execute", [executable], env=env)
                    row["cpp_observer_source"] = observer.read_text()
                    row["cpp_descriptors"] = json.loads(observation.stdout)
                    expected = [{key: value for key, value in descriptor.items() if key not in ("module", "type")}
                                for descriptor in job["descriptors"]]
                    require(row["cpp_descriptors"] == expected, "complete actual original/C++ descriptors differ")
                    write(path, record)
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(owned_directory_removed=not Path(record["owned_directory"]).exists(),
                      input_hashes_after=capture_inputs(), references_after=references())
        require(record["input_hashes_before"] == record["input_hashes_after"]
                and record["references_before"] == record["references_after"], "stable actual source inputs")
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
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["owned_directory_removed"] is True, "complete bounded worker qualification")
    require([row["name"] for row in record["cases"]]
            == ["configured", "changed_include", "changed_environment", "zero_relations"], "four actual jobs")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"] == {
                "teaql-rs": "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3",
                "teaql_code_gen": "45e1572c4956ca963410d8ea436ead3c8857087a"}, "stable locked source inputs")
    require(record["worker_source"] == WORKER.read_text() and record["bridge_source"] == BRIDGE.read_text()
            and record["driver_source"] == Path(__file__).read_text()
            and all(sha(Path(name).read_bytes()) == value for name, value in record["java_sources"].items()),
            "exact executed worker, bridge, driver and compiled Java sources")
    spec = importlib.util.spec_from_file_location("qualified_cpp_compiler_job", WORKER)
    worker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(worker)
    import rust_configuration_protocol as protocol
    labels = [step["label"] for step in record["steps"]]
    expected_labels = ["original_javac", "cpp_javac", "unit_controls_0", "unit_controls_1",
                       "configured_job", "configured_native_compile", "configured_native_execute",
                       "changed_include_job", "changed_environment_job", "zero_relations_job",
                       "zero_relations_native_compile", "zero_relations_native_execute"]
    require(labels == expected_labels and all(type(step["exit_code"]) is int and step["exit_code"] == (
            2 if step["label"] in ("changed_include_job", "changed_environment_job") else 0)
            for step in record["steps"]), "complete actual Java/unit/CLI/native process membership")
    for row in record["cases"]:
        job = row["job"]
        require(job["complete"] is True and job["owned_directory_removed"] is True
                and job["publication_directory_removed"] is True
                and job["original_sources_before"] == job["original_sources_after"], "unchanged original sources and cleanup")
        require(job["model_sha256"] == sha(row["model"].encode())
                and job["worker_sha256"] == sha(record["worker_source"].encode())
                and job["toolchain"] == json.loads(TOOLCHAIN.read_text()), "actual model/worker/compiler binding")
        compiled = [step for step in job["steps"] if step["label"] == "original_compile"]
        require(len(compiled) == 1 and type(compiled[0]["exit_code"]) is int, "one actual original compile per job")
        if row["cli_exit_code"]:
            require(row["cli_exit_code"] == 2 and compiled[0]["exit_code"] == 101
                    and job["outcome"] == "compiler_refusal" and job["compiler_errors"]
                    and all(step["label"] != "cpp_generate" for step in job["steps"]), "real refusal without generated SDK")
        else:
            require(compiled[0]["exit_code"] == 0 and job["outcome"] == "resolved"
                    and job["resolution_document"]["outcome"] == "resolved"
                    and len(job["observer_artifacts"]) == 3, "same-artifact descriptor/projection generation")
            require(any(step["label"] == "observer_compile" and step["exit_code"] == 0 for step in job["steps"])
                    and any(step["label"] == "original_descriptors" and step["exit_code"] == 0 for step in job["steps"])
                    and any(step["label"] == "cpp_generate" and step["exit_code"] == 0 for step in job["steps"]),
                    "actual observer and producer completion")
            require(sha(job["mir"].encode()) == job["mir_sha256"]
                    and worker.collect_projections(protocol, job["mir"], job["original_inventory"]["entities"],
                                                   job["descriptors"], Path(job["owned_directory"]) / "original")
                    == job["protocol"], "replayed complete actual compiler projection inventory")
            document = job["resolution_document"]
            require(document["model_sha256"] == job["model_sha256"]
                    and document["compiler_lock_sha256"] == sha(TOOLCHAIN.read_bytes())
                    and document["source_manifest"] == job["original_sources_before"]
                    and document["source_manifest_sha256"] == worker.manifest_sha(document["source_manifest"])
                    and document["mir_sha256"] == job["mir_sha256"]
                    and document["dependency_lock_sha256"] == sha(job["dependency_lock"].encode())
                    and document["compiler_environment_sha256"] == job["compiler_environment_sha256"]
                    and document["projections"] == [{key: fact[key] for key in (
                        "source_module", "owner", "relation", "cardinality", "resolved_type")}
                        for fact in job["protocol"]["projections"]], "exact document consumed by C++ generation")
            require(row["cpp_descriptors"] == [{key: value for key, value in d.items() if key not in ("module", "type")}
                                               for d in job["descriptors"]], "all actual C++ descriptor outcomes")
    configured, included, environment, empty = record["cases"]
    require(configured["job"]["original_inventory"]["package"] == "router-capability-core"
            and any(d["owner"] == "SensorWire" for d in configured["job"]["descriptors"])
            and any(p["owner"] == "SensorWire" for p in configured["job"]["protocol"]["projections"]),
            "arbitrary package/entity inventory and compiled owner override")
    require(any(name.endswith("/external_count.rs") and value == sha(configured["included_source"].encode())
                for name, value in configured["job"]["compiler_source_inputs"].items()), "actual include dependency bytes")
    require(configured["model"] == included["model"] == environment["model"]
            and any(error["code"] == "E0308" for error in included["job"]["compiler_errors"])
            and any(error["code"] == "E0080" for error in environment["job"]["compiler_errors"]),
            "fresh compilation observes changed included data and environment")
    require(empty["job"]["protocol"]["projections"] == []
            and empty["job"]["protocol"]["relation_call_count"] == 0
            and len(empty["cpp_descriptors"]) == 1, "valid zero-relation domain")


def controls(path):
    valid = read(path)
    validate(valid)
    result = []
    mutations = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_job", lambda r: r["cases"].pop()),
        ("invented_compile", lambda r: next(s for s in r["cases"][0]["job"]["steps"] if s["label"] == "original_compile").update(exit_code=True)),
        ("mutated_original", lambda r: r["cases"][0]["job"]["original_sources_after"].update(invented="0" * 64)),
        ("lost_include_dependency", lambda r: r["cases"][0]["job"].update(compiler_source_inputs={})),
        ("wrong_descriptor", lambda r: r["cases"][0]["cpp_descriptors"][0].update(owner="invented")),
        ("stale_include", lambda r: r["cases"][1]["job"].update(compiler_errors=[])),
        ("ignored_environment", lambda r: r["cases"][2]["job"]["compiler_errors"][0].update(code="invented")),
        ("dropped_zero_model", lambda r: r["cases"][3]["job"]["protocol"].update(relation_call_count=1)),
        ("missing_native_process", lambda r: r["steps"].pop(5)),
        ("missing_optimized_unit_gate", lambda r: r["steps"].pop(3)),
        ("changed_worker_source", lambda r: r.update(worker_source="pass\n")),
        ("wrong_model_binding", lambda r: r["cases"][0]["job"].update(model_sha256="0" * 64)),
        ("changed_mir", lambda r: r["cases"][0]["job"].update(mir="unknown MIR")),
        ("wrong_mapping_document", lambda r: r["cases"][0]["job"]["resolution_document"]["projections"].pop()),
        ("wrong_compiler_lock", lambda r: r["cases"][0]["job"]["resolution_document"].update(compiler_lock_sha256="0" * 64)),
    )
    for name, mutation in mutations:
        changed = copy.deepcopy(valid)
        mutation(changed)
        try:
            validate(changed)
        except RuntimeError as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("False acceptance: " + name)
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
        require(not args.output.exists(), "preserve existing controls")
        result = controls(args.controls)
        write(args.output, result)
        print(json.dumps({"complete": True, "optimized": result["optimized"], "controls": len(result["controls"])}))
    else:
        parser.error("Choose --execute or --controls with --output")


if __name__ == "__main__":
    main()
