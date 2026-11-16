#!/usr/bin/env python3
"""Execute compiler-normalized scalar storage through complete generated SDKs."""

import argparse
import base64
import copy
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from diagnose_compiler_scalar_storage_5_0_6 import cases as diagnostic_cases
from verify_compiler_generated_business_5_0_6 import (
    execution_inputs, manifest, native_command, read, references, require, run_owned, sha, write,
)
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_model_configuration_5_0_6 import worker_module
from verify_compiler_projection_generation_5_0_6 import hashes
from verify_audit_metadata_source_5_0_6 import identities

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
CPP = ROOT / "tests/parity/compiler_scalar_storage_probe.cpp"
RUST = ROOT / "tests/parity/compiler_scalar_storage_probe.rs"
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same


def cases():
    result = diagnostic_cases()
    carriers = ["std::int64_t", "double", "bool"]
    values = [[4294967297, -4294967297], [2.5, -3.75], [True, False]]
    for case, carrier, data in zip(result, carriers, values, strict=True):
        case.update(carrier=carrier, values=data, expected_exit=0)
    result.extend([
        dict(name="qualified_float", model=result[1]["model"].replace('name="double"', 'name="float"')
             .replace("::core::primitive::f64", "::core::primitive::f32").replace('value="double()"', 'value="float()"'),
             carrier="float", values=[1.5, -3.25], expected_exit=0),
        dict(name="optional_double", model=result[1]["model"].replace('value="double()"', 'value="double()?"'),
             carrier="std::optional<double>", values=[2.5, None], expected_exit=0),
        dict(name="source_refusal", model=result[0]["model"].replace("::core::primitive::i64", "u64"), expected_exit=2),
    ])
    return [result[4], *result[:4], result[5]]


def inputs(case):
    schema = ("CREATE TABLE gateway_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);"
              "CREATE TABLE reading_data(id INTEGER PRIMARY KEY,value, gateway INTEGER,version INTEGER);"
              "INSERT INTO gateway_data VALUES(1,'edge',1);")
    values = ["NULL" if value is None else str(int(value)) if isinstance(value, bool) else str(value)
              for value in case["values"]]
    populated = schema + "INSERT INTO reading_data VALUES" + ",".join(
        "(" + str(index + 7) + "," + value + ",1,1)" for index, value in enumerate(values)) + ";"
    return [dict(sql=populated), dict(sql=schema)]


def execute(path):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh scalar receipt and owned output space")
    require(CPP.is_file() and RUST.is_file(), "complete native/reference consumer sources")
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  worker_source=WORKER.read_text(), preparation_steps=[], steps=[], cases=[],
                  inputs_before=execution_inputs(), references_before=references())
    owned = None

    def run(label, command, *, env, input_text=None, expected=0):
        command = list(map(str, command))
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=input_text, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, exit_code=result.returncode,
                                    input=input_text, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        require(type(result.returncode) is int and result.returncode == expected, label + " failed; inspect retained stdout/stderr")
        print(label, "exit", result.returncode, flush=True)
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_scalar_storage_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = temporary
            try:
                capability_path, classpath = prepare(owned, record)
                config = record["capabilities"]
                config["schema"] = "teaql.rust_configuration.job.v3"
                capability_path.write_text(json.dumps(config, sort_keys=True))
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                           CARGO_TARGET_DIR=str(owned / "consumer_target"),
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                           TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                for case in cases():
                    name = case["name"]
                    directory = owned / name
                    directory.mkdir()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(case["model"].encode()).decode()})
                    source, cpp, report = directory / "model.json", directory / "cpp", directory / "job.json.gz"
                    source.write_text(json.dumps(package, sort_keys=True))
                    row = dict(case, source_package=package)
                    record["cases"].append(row)
                    run(name + "_controller", [config["cpp_generator"]["command"][0], "-cp", classpath,
                        "CppCompilerServiceProbe", source, cpp, Path(sys.executable).resolve(), WORKER,
                        capability_path, owned / "service_jobs", report, "--controller"], env=env, expected=case["expected_exit"])
                    row.update(job=read(report), sdk_exists=cpp.exists(),
                               service_jobs_remaining=list(map(str, (owned / "service_jobs").iterdir())),
                               compiler_jobs_remaining=list(map(str, (owned / "compiler_jobs").iterdir())))
                    if case["expected_exit"]:
                        continue
                    raw = base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True)
                    require(json.loads(raw) == package, "same controller model bytes")
                    source.write_bytes(raw)
                    original, inventory = directory / "original", directory / "inventory.json"
                    run(name + "_original_generate", [*config["reference_generator"]["command"],
                        source, original, inventory, "--source-package", "--scalar-types"], env=env)
                    row.update(original_sources=hashes(original), original_inventory=json.loads(inventory.read_bytes()),
                               cpp_sources=hashes(cpp), provenance=(cpp / "RUST_CONFIGURATION.json").read_text())
                    require(row["original_sources"] == row["job"]["original_sources_before"], "business and compiler use the same complete original SDK")
                    native = owned / "native_probe"
                    command = native_command(cpp / "include", CPP, native)
                    command[1:1] = ["-I" + str(ROOT / "tests/parity"), "-DTEAQL_EXPECTED_VALUE_TYPE=" + case["carrier"]]
                    run(name + "_native_build", command, env=env)
                    harness = directory / "harness"
                    harness.mkdir()
                    text = manifest(original, "domain", RUST)
                    (harness / "Cargo.toml").write_text(text)
                    shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                    run(name + "_original_resolve", ["cargo", "metadata", "--offline", "--format-version", "1",
                        "--manifest-path", harness / "Cargo.toml"], env=env)
                    require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "reference dependency identities")
                    run(name + "_original_build", ["cargo", "build", "--offline", "--locked", "--manifest-path", harness / "Cargo.toml", "-j", "2"], env=env)
                    payload = "".join(json.dumps(value) + "\n" for value in inputs(case))
                    row["original"] = json_lines(run(name + "_original_run", [owned / "consumer_target/debug/compiler_generated_business_probe"], env=env, input_text=payload))
                    row["cpp"] = json_lines(run(name + "_native_run", [native], env=env, input_text=payload))
                    row.update(consumer_manifest=text, consumer_lock=(harness / "Cargo.lock").read_text(),
                               original_sources_after=hashes(original), cpp_sources_after=hashes(cpp))
                    require(same(row["original"], row["cpp"]), "actual scalar business outcomes differ: " + name)
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(),
                      inputs_after=execution_inputs(), references_after=references())
        write(path, record)
    validate(record)


def validate(record):
    from compiler_scalar_source_admission_5_0_6 import registered_epoch_matches, frame_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and registered_epoch_matches(Path(__file__), sha(record["driver_source"].encode()))
            and registered_epoch_matches(WORKER, sha(record["worker_source"].encode())), "complete exact scalar execution implementation")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"] == references(), "stable source frame, references and owned cleanup")
    current = execution_inputs()
    require(frame_matches(current, record["inputs_after"]), "actual current scalar sources")
    require(record["capabilities"]["schema"] == "teaql.rust_configuration.job.v3"
            and [s["label"] for s in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
            and all(type(s["exit_code"]) is int and s["exit_code"] == 0 for s in record["preparation_steps"]), "explicit scalar capability and two fresh Java compilations")
    require([c["name"] for c in record["cases"]] == [c["name"] for c in cases()], "complete scalar matrix")
    worker = worker_module()
    labels = []
    for row, expected in zip(record["cases"], cases(), strict=True):
        require(all(row[key] == value for key, value in expected.items()), "exact scalar model and native carrier")
        name, job = row["name"], row["job"]
        labels.append(name + "_controller")
        require(row["service_jobs_remaining"] == row["compiler_jobs_remaining"] == []
                and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and job["worker_sha256"] == sha(record["worker_source"].encode())
                and job["capabilities_sha256"] == sha(json.dumps(record["capabilities"], sort_keys=True).encode()), "actual worker and complete cleanup")
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(json.loads(raw) == row["source_package"] and sha(raw) == job["model_sha256"], "same actual compiler package")
        steps = {s["label"]: s for s in job["steps"]}
        if expected["expected_exit"]:
            require(job["complete"] is True and job["outcome"] == "compiler_refusal" and row["sdk_exists"] is False
                    and steps["original_compile"]["exit_code"] == 101 and job["compiler_errors"], "original compiler refusal without SDK or type fallback")
            continue
        require(job["complete"] is True and job["outcome"] == "resolved" and row["sdk_exists"] is True, "actual resolved complete scalar SDK")
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        worker.validate_scalar_types(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
        document = job["resolution_document"]
        require(document["schema"] == "teaql.rust_configuration.model_generation.v2"
                and same(document["scalar_types"], job["scalar_types"])
                and document["scalar_observer_sha256"] == sha(job["scalar_observer_source"].encode())
                and document["scalar_artifacts_sha256"] == worker.manifest_sha(job["scalar_artifacts"])
                and job["scalar_artifacts"] == job["configuration_artifacts"] == job["observer_artifacts"], "scalar, descriptors and MIR use the same compiled original library")
        require(job["scalar_observer_source"] == worker.scalar_source(job["original_inventory"]["entities"])
                and same(json.loads(steps["original_scalar_types"]["stdout"]), job["scalar_types"])
                and all(type(steps[k]["exit_code"]) is int and steps[k]["exit_code"] == 0 for k in
                        ("original_compile", "scalar_observer_compile", "original_scalar_types", "cpp_generate")), "real complete compiler and scalar observer execution")
        require(row["original_inventory"] == job["original_inventory"]
                and row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"]
                and row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"]
                and json.loads(row["provenance"])["configuration_resolution_sha256"] == sha(json.dumps(document, sort_keys=True).encode()), "actual immutable SDKs and new scalar provenance")
        actual = {s["label"]: s for s in record["steps"]}
        suffixes = ("original_generate", "native_build", "original_resolve", "original_build", "original_run", "native_run")
        labels.extend(name + "_" + suffix for suffix in suffixes)
        require(actual[name + "_controller"]["command"][-1] == "--controller"
                and "-DTEAQL_EXPECTED_VALUE_TYPE=" + expected["carrier"] in actual[name + "_native_build"]["command"], "controller entry and actual compile-time C++ storage identity")
        payload = "".join(json.dumps(value) + "\n" for value in inputs(expected))
        require(actual[name + "_original_run"]["input"] == actual[name + "_native_run"]["input"] == payload
                and row["original"] == json_lines(actual[name + "_original_run"]["stdout"])
                and row["cpp"] == json_lines(actual[name + "_native_run"]["stdout"])
                and len(row["original"]) == len(row["cpp"]) == 2 and same(row["original"], row["cpp"]), "all actual scalar query/getter/E/JSON and SQL results match")
        require([v["getter"] for v in row["original"][0]["rows"]] == expected["values"]
                and row["original"][1]["rows"] == [], "nonempty typed values, optional NULL and empty selection are actually exercised")
    require([s["label"] for s in record["steps"]] == labels
            and all(type(s["exit_code"]) is int and s["exit_code"] == (2 if s["label"] == "source_refusal_controller" else 0) for s in record["steps"]), "all actual terminal process membership")


def controls(record):
    validate(record)
    changes = (
        ("unfinished", lambda r: r.update(complete=False)),
        ("old_capability", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v2")),
        ("missing_model", lambda r: r["cases"].pop()),
        ("wrong_carrier", lambda r: r["cases"][0].update(carrier="double")),
        ("missing_scalar_field", lambda r: r["cases"][0]["job"]["scalar_types"][0]["properties"].pop()),
        ("wrong_scalar_type", lambda r: r["cases"][0]["job"]["scalar_types"][0]["properties"][0].update(rust_type=False)),
        ("wrong_library", lambda r: r["cases"][0]["job"]["scalar_artifacts"].update(invented="0" * 64)),
        ("missing_scalar_compile", lambda r: r["cases"][0]["job"]["steps"].pop(next(i for i,s in enumerate(r["cases"][0]["job"]["steps"]) if s["label"] == "scalar_observer_compile"))),
        ("changed_observer", lambda r: r["cases"][0]["job"].update(scalar_observer_source="fn main() {}\n")),
        ("missing_output", lambda r: r["cases"][0]["cpp"].pop()),
        ("wrong_input", lambda r: r["steps"][-2].update(input="{}\n")),
        ("changed_raw_output", lambda r: r["steps"][-2].update(stdout="{}\n")),
        ("hidden_source_refusal", lambda r: r["cases"][-1].update(sdk_exists=True)),
        ("bool_exit", lambda r: r["steps"][0].update(exit_code=False)),
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
            raise RuntimeError("Accepted scalar corruption: " + name)
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
            require(not args.controls.exists(), "preserve scalar control receipts")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                                      execution_sha256=sha(args.record.read_bytes()), controls=result))
        print("Verified scalar storage:", len(result), "controls; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
