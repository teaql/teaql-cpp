#!/usr/bin/env python3
"""Exercise strong Business IDs through complete locked compiler generation."""

import argparse
import base64
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

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
PARITY = ROOT / "tests/parity"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
MODELS = ("business_ids", "inheritance_business_ids")
SOURCES = {
    "checker": (PARITY / "loaded_snapshot_probe.cpp", PARITY / "rust_business_id_helper_closure_5_0_6/src/main.rs"),
    "expression": (PARITY / "expression_business_ids_probe.cpp", PARITY / "rust_expression_business_ids_5_0_6/src/main.rs"),
    "lookup": (PARITY / "business_id_lookups_probe.cpp", PARITY / "rust_expression_business_ids_5_0_6/src/bin/business_id_lookups.rs"),
    "save": (ROOT / "tests/generator/generated_business_id_tests.cpp", None),
}
CORPORA = {
    "checker": ("business_ids_5_0_6_cases.json.gz", 1044),
    "expression": ("expression_business_ids_5_0_6_cases.json.gz", 96),
    "lookup": ("business_id_lookups_5_0_6_cases.json.gz", 480),
}
sys.path.insert(0, str(PARITY))
from compare_values import json_lines, same
from compare_expression_business_ids_5_0_6 import adapted
from generate_business_ids import adapt_source


def cases(name):
    filename, count = CORPORA[name]
    result = read(PARITY / filename)["cases"]
    if name == "lookup":
        result = [row for row in result if row["input"]["entity"] != "document"]
    require(len(result) == count, "complete regular Business ID corpus: " + name)
    return result


def adapted_source(name, owned):
    source = SOURCES[name][0].read_text()
    before = '#include "../generated_business_ids/include/cpp_business_ids.hpp"'
    after = '#include "' + str(owned / "business_ids/cpp/include/cpp_business_ids.hpp") + '"'
    require(source.count(before) == 1, "one Business ID generated include")
    return source.replace(before, after, 1), dict(before=before, after=after)


def consumer_manifest(owned, inventory):
    source = SOURCES["checker"][1]
    base = manifest(owned / "business_ids/original", "cpp_business_ids_core", source, inventory["package"])
    header, tail = base.split("[[bin]]\n", 1)
    _, dependencies = tail.split("[dependencies]\n", 1)
    build = PARITY / "rust_business_id_helper_closure_5_0_6/build.rs"
    header = header.replace('[workspace]\n', 'build=' + json.dumps(str(build)) + '\n[workspace]\n')
    bins = "".join('[[bin]]\nname="business_' + name + '"\npath='
                   + json.dumps(str(SOURCES[name][1])) + '\n' for name in CORPORA)
    return header + bins + '[dependencies]\nchrono="=0.4.45"\n' + dependencies


def commands(record):
    owned, config = Path(record["owned_directory"]), record["capabilities"]
    command = config["cpp_generator"]["command"]
    classpath = command[command.index("-cp") + 1]
    result = {}
    for name in MODELS:
        current = owned / name
        result[name + "_controller"] = [command[0], "-cp", classpath, "CppCompilerServiceProbe",
            str(current / "model.json"), str(current / "cpp"), str(Path(sys.executable).resolve()), str(WORKER),
            str(owned / "capabilities.json"), str(owned / "service_jobs"), str(current / "job.json.gz"), "--controller"]
        result[name + "_original_generate"] = [*config["reference_generator"]["command"], str(current / "model.json"),
            str(current / "original"), str(current / "inventory.json"), "--source-package", "--semantic-storage"]
    path = str(owned / "consumer/Cargo.toml")
    result["original_consumer_resolve"] = ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", path]
    result["original_consumers_build"] = ["cargo", "build", "--offline", "--locked", "--bins", "-j", "2", "--manifest-path", path]
    for name in SOURCES:
        command = native_command(owned / "business_ids/cpp/include", owned / (name + ".cpp"), owned / ("native_" + name))
        command[1:1] = ["-I" + str(PARITY), "-DTEAQL_SOURCE_DIR=" + json.dumps(str(ROOT))]
        result[name + "_native_build"] = list(map(str, command))
        if name in CORPORA:
            result[name + "_original_run"] = [str(owned / "consumer_target/debug" / ("business_" + name))]
        result[name + "_native_run"] = [str(owned / ("native_" + name))]
    return result


def compare(name, original, native):
    rows = cases(name)
    require(len(original) == len(native) == len(rows), "complete actual Business ID outputs")
    expected = [row["source_expected" if name == "checker" else "expected"] for row in rows]
    require(same(original, expected), "complete original Business ID results: " + name)
    if name == "checker":
        require(same(native, [adapt_source(row) for row in original]), "typed Checkers with declared resource type-name presentation")
    elif name == "expression":
        require(same(original, [adapted(row) for row in native]), "typed E values and declared raw-diagnostic presentation")
    else:
        require(same(native, original), "actual typed Q/SQLite/JSON lookup outcomes")


def execute(path):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh receipt and owned output space")
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
        worker_source=WORKER.read_text(), inputs_before=execution_inputs(), references_before=references(),
        native_inputs_before=native_inputs(), preparation_steps=[], steps=[], models=[], consumers=[])
    owned = None

    def run(label, command, *, env=None, input_text=None, allowed=(0,)):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=input_text, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=list(map(str, command)), input=input_text,
            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode in allowed, label + " failed; inspect retained output")
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_business_id_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capability_path, _ = prepare(owned, record)
                config = record["capabilities"]
                config["schema"] = "teaql.rust_configuration.job.v4"
                capability_path.write_text(json.dumps(config, sort_keys=True))
                calls = commands(record)
                for name in MODELS:
                    current = owned / name
                    current.mkdir()
                    model = (ROOT / "tests/generator" / (name + ".xml")).read_text()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(model.encode()).decode()})
                    (current / "model.json").write_text(json.dumps(package, sort_keys=True))
                    row = dict(name=name, model=model, source_package=package)
                    record["models"].append(row)
                    run(name + "_controller", calls[name + "_controller"], allowed=(0, 2))
                    row["job"] = read(current / "job.json.gz")
                    require(row["job"]["outcome"] in ("resolved", "compiler_refusal"), "actual mapping or original compiler refusal")
                    require(name != "business_ids" or row["job"]["outcome"] == "resolved", "regular Business ID mapping must succeed")
                    run(name + "_original_generate", calls[name + "_original_generate"])
                    row.update(inventory=json.loads((current / "inventory.json").read_text()),
                        original_sources=hashes(current / "original"), cpp_sources=hashes(current / "cpp"))
                    if row["job"]["outcome"] == "resolved":
                        row["provenance"] = json.loads((current / "cpp/RUST_CONFIGURATION.json").read_bytes())
                harness = owned / "consumer"
                harness.mkdir()
                original_source = PARITY / "rust_field_layout/src/main.rs"
                copied = owned / "rust_field_layout/src/main.rs"
                copied.parent.mkdir(parents=True)
                copied.write_bytes(original_source.read_bytes())
                record["copied_original_observer"] = original_source.read_text()
                text = consumer_manifest(owned, record["models"][0]["inventory"])
                (harness / "Cargo.toml").write_text(text)
                record["consumer_manifest"] = text
                shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                    CARGO_TARGET_DIR=str(owned / "consumer_target"), TEAQL_LOG_ENDPOINT="off",
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                    TEAQL_BUSINESS_ID_KEY_VERSION="1", TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
                record["environment_overrides"] = {key: env[key] for key in (
                    "CARGO_INCREMENTAL", "CARGO_PROFILE_DEV_DEBUG", "CARGO_TARGET_DIR", "TEAQL_LOG_ENDPOINT",
                    "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER",
                    "TEAQL_BUSINESS_ID_KEY_VERSION", "TEAQL_BUSINESS_ID_KEY_V1_HEX")}
                run("original_consumer_resolve", calls["original_consumer_resolve"], env=env)
                require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "locked dependency identities")
                run("original_consumers_build", calls["original_consumers_build"], env=env)
                record["consumer_lock"] = (harness / "Cargo.lock").read_text()
                for name in SOURCES:
                    source, edit = adapted_source(name, owned)
                    (owned / (name + ".cpp")).write_text(source)
                    row = dict(name=name, cpp_source=SOURCES[name][0].read_text(), adapted_source=source, include_edit=edit)
                    record["consumers"].append(row)
                    run(name + "_native_build", calls[name + "_native_build"], env=env)
                    if name in CORPORA:
                        data = "".join(json.dumps(value["input"], ensure_ascii=True) + "\n" for value in cases(name))
                        row["original"] = json_lines(run(name + "_original_run", calls[name + "_original_run"], env=env, input_text=data))
                        row["cpp"] = json_lines(run(name + "_native_run", calls[name + "_native_run"], env=env, input_text=data))
                        compare(name, row["original"], row["cpp"])
                    else:
                        row["output"] = run(name + "_native_run", calls[name + "_native_run"], env=env)
                record["artifacts_after"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                record["service_jobs_remaining"] = list(map(str, (owned / "service_jobs").iterdir()))
                record["compiler_jobs_remaining"] = list(map(str, (owned / "compiler_jobs").iterdir()))
                record["complete"] = True
            finally:
                for row in record["models"]:
                    current = owned / row["name"]
                    if (current / "job.json.gz").is_file():
                        row["job"] = read(current / "job.json.gz")
                    row.update(original_sources_after=hashes(current / "original"), cpp_sources_after=hashes(current / "cpp"))
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(), inputs_after=execution_inputs(),
            references_after=references(), native_inputs_after=native_inputs())
        write(path, record)
    validate(record)


def execution_source_matches(source):
    import ast

    current = Path(__file__).read_text()
    if source == current:
        return True
    if sha(source.encode()) != "d136c04ed37326638e979507ba754e380910c1c1bc11545e140c4d64808bcddb":
        return False
    before, after = ast.parse(source), ast.parse(current)
    for tree in (before, after):
        tree.body = [node for node in tree.body if not isinstance(node, ast.FunctionDef)
                     or node.name not in ("validate", "execution_source_matches")]
    return ast.dump(before, include_attributes=False) == ast.dump(after, include_attributes=False)


def validate(record):
    from compiler_business_id_source_admission_5_0_6 import registered_epoch_matches, frame_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False, "bounded completed execution receipt")
    require(registered_epoch_matches(Path(__file__), sha(record["driver_source"].encode()))
            and registered_epoch_matches(WORKER, sha(record["worker_source"].encode())), "exact execution bodies and worker")
    current = execution_inputs()
    require(record["inputs_before"] == record["inputs_after"] and frame_matches(current, record["inputs_after"]),
            "unchanged complete execution inputs with exact registered source continuations")
    require(record["references_before"] == record["references_after"] == references(), "locked reference identities")
    require(record["native_inputs_before"] == record["native_inputs_after"] == native_inputs(), "unchanged runtime/compiler/SQLite inputs")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "owned output cleanup")
    require([s["label"] for s in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
            and all(type(s["exit_code"]) is int and s["exit_code"] == 0 for s in record["preparation_steps"]), "fresh current/original Java compilation")
    prepared = {name for step in record["preparation_steps"] for name in step["command"] if name.endswith(".java")}
    require(set(record["prepared_java_sources"]) == prepared
            and all(registered_epoch_matches(Path(name), digest) for name, digest in record["prepared_java_sources"].items()), "complete prepared Java source identities")
    config = record["capabilities"]
    require(config["schema"] == "teaql.rust_configuration.job.v4" and config["artifacts"] == record["artifacts_after"], "semantic capability and immutable Java artifacts")
    require(record["environment_overrides"] == dict(CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
        CARGO_TARGET_DIR=str(Path(record["owned_directory"]) / "consumer_target"),
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"], TEAQL_LOG_ENDPOINT="off",
        TEAQL_BUSINESS_ID_KEY_VERSION="1", TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex()), "locked consumer linker and explicit process settings")
    require(record["environment_overrides"]["CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"] == config["linker"]["path"], "consumer uses the locked linker")
    expected = commands(record)
    require([s["label"] for s in record["steps"]] == list(expected), "complete ordered actual command inventory")
    for step in record["steps"]:
        require(step["command"] == expected[step["label"]] and type(step["exit_code"]) is int
                and step["exit_code"] in ((0, 2) if step["label"] == "inheritance_business_ids_controller" else (0,)), "exact successful invocation or original inheritance refusal")
    worker = worker_module()
    require([row["name"] for row in record["models"]] == list(MODELS), "complete original model membership")
    for row in record["models"]:
        model, job = row["name"], row["job"]
        require(row["model"] == (ROOT / "tests/generator" / (model + ".xml")).read_text(), "unchanged complete model")
        package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                       files={"main.xml": base64.b64encode(row["model"].encode()).decode()})
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(row["source_package"] == package and worker.strict_json(raw) == package
                and job["model_sha256"] == sha(raw)
                and row["inventory"]["model_sha256"] == sha(json.dumps(package, sort_keys=True).encode()), "both exact JSON envelopes carry identical original model bytes")
        require(job["complete"] is True and job["worker_sha256"] == sha(record["worker_source"].encode())
                and job["capabilities_sha256"] == sha(json.dumps(config, sort_keys=True).encode())
                and job["artifacts"] == config["artifacts"] and job["references_before"] == job["references_after"], "same worker and prepared original/current libraries")
        require(job["original_inventory"]["model_sha256"] == job["model_sha256"]
                and {key: value for key, value in job["original_inventory"].items() if key != "model_sha256"}
                == {key: value for key, value in row["inventory"].items() if key != "model_sha256"}
                and row["inventory"]["schema"] == "teaql.rust_configuration.model.v3", "independent original inventory with each envelope's actual hash")
        require(job["original_sources_before"] == job["original_sources_after"] == row["original_sources"] == row["original_sources_after"], "same complete original SDK in compiler job and consumers")
        require(row["cpp_sources"] == row["cpp_sources_after"], "unchanged complete C++ SDK")
        if job["outcome"] == "resolved":
            require(job["cpp_sources"] == row["cpp_sources"] and bool(row["cpp_sources"]), "complete published C++ SDK")
            worker.validate_configuration(row["inventory"]["entities"], job["descriptors"], job["configuration"])
            worker.validate_semantic_storage(row["inventory"]["entities"], job["configuration"], job["scalar_types"])
            document = job["resolution_document"]
            require(document["schema"] == "teaql.rust_configuration.model_generation.v3"
                    and document["scalar_types"] == job["scalar_types"] and document["entities"] == job["configuration"]
                    and document["scalar_observer_sha256"] == sha(job["scalar_observer_source"].encode())
                    and job["scalar_observer_source"] == worker.semantic_storage_source(row["inventory"]["entities"])
                    and document["scalar_artifacts_sha256"] == worker.manifest_sha(job["scalar_artifacts"])
                    and job["scalar_artifacts"] == job["configuration_artifacts"] == job["observer_artifacts"]
                    and row["provenance"]["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode()), "same-library explicit semantic document and SDK provenance")
            scalar = next(s for s in job["steps"] if s["label"] == "original_scalar_types")
            metadata = next(s for s in job["steps"] if s["label"] == "original_configuration")
            require(worker.strict_json(scalar["stdout"]) == job["scalar_types"]
                    and worker.strict_json(metadata["stdout"]) == job["configuration"]
                    and all(type(s["exit_code"]) is int and s["exit_code"] == 0 for s in job["steps"]), "actual compiled semantic and descriptor observations")
            require(job["scalar_observer_source"] == worker.semantic_storage_source(row["inventory"]["entities"]), "actual nominal-identity observer program")
        else:
            prior = json.loads((ROOT / "docs/parity/inherited_business_source_failure_5_0_6.json").read_bytes())
            require(model == "inheritance_business_ids" and job["outcome"] == "compiler_refusal"
                    and sorted((e["code"], e["message"]) for e in job["compiler_errors"])
                    == sorted((e["code"], e["message"]) for e in prior["errors"])
                    and not row["cpp_sources"], "exact previously retained twenty-error refusal, no inferred inheritance parity")
        controller = next(s for s in record["steps"] if s["label"] == model + "_controller")
        require(controller["exit_code"] == (0 if job["outcome"] == "resolved" else 2), "controller and original compiler outcome agree")
        require(job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and not Path(job["owned_directory"]).exists() and not Path(job["publication_directory"]).exists(), "worker cleanup before publication")
    require(record["consumer_manifest"] == consumer_manifest(Path(record["owned_directory"]), record["models"][0]["inventory"]), "authentic consumer dependency assembly")
    require(record["service_jobs_remaining"] == record["compiler_jobs_remaining"] == [], "all generation-owned jobs removed")
    require(record["copied_original_observer"] == (PARITY / "rust_field_layout/src/main.rs").read_text(), "unchanged original Checker observer closure")
    require(identities(SimpleNamespace(read_text=lambda: record["consumer_lock"])) <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "locked consumer dependency identities")
    require([row["name"] for row in record["consumers"]] == list(SOURCES), "all named consumers executed")
    for row in record["consumers"]:
        name = row["name"]
        source, edit = adapted_source(name, Path(record["owned_directory"]))
        require(row["cpp_source"] == SOURCES[name][0].read_text() and row["adapted_source"] == source and row["include_edit"] == edit, "include-only C++ adapter, unchanged observer bodies")
        if name in CORPORA:
            data = "".join(json.dumps(value["input"], ensure_ascii=True) + "\n" for value in cases(name))
            original = next(s for s in record["steps"] if s["label"] == name + "_original_run")
            native = next(s for s in record["steps"] if s["label"] == name + "_native_run")
            require(original["input"] == native["input"] == data and json_lines(original["stdout"]) == row["original"]
                    and json_lines(native["stdout"]) == row["cpp"], "complete authentic raw observer input and output")
            compare(name, row["original"], row["cpp"])
        else:
            native = next(s for s in record["steps"] if s["label"] == "save_native_run")
            require(row["output"] == native["stdout"] == "Generated Business-ID contracts passed\n", "actual native typed allocation and governed SQLite Save")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path)
    parser.add_argument("--check", type=Path)
    args = parser.parse_args()
    require(bool(args.execute) != bool(args.check), "choose execute or check")
    if args.execute:
        execute(args.execute)
    else:
        validate(read(args.check))


if __name__ == "__main__":
    main()
