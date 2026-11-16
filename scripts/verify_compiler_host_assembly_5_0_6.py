#!/usr/bin/env python3
"""Execute complete compiler-generated SDKs through ordinary Spring host assembly."""

import argparse
import base64
import copy
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from verify_audit_metadata_source_5_0_6 import identities
import verify_compiler_business_id_storage_5_0_6 as business
from verify_compiler_generated_business_5_0_6 import (
    execution_inputs, manifest, native_inputs, read, references, require, run_owned, sha, write,
)
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_scalar_storage_5_0_6 import cases as scalar_cases, inputs as scalar_inputs
from verify_compiler_source_service_5_0_6 import artifact_sha

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
PARITY = ROOT / "tests/parity"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
HOST = PRODUCER / "tools/cpp_compiler_service/CppCompilerHostProbe.java"
sys.path.insert(0, str(PARITY))
from compare_values import json_lines, same


def models():
    scalar = next(row for row in scalar_cases() if row["name"] == "qualified_integer")
    return [dict(name="scalar", model=scalar["model"], expected_exit=0, controller=True),
            dict(name="business_ids", model=(ROOT / "tests/generator/business_ids.xml").read_text(),
                 expected_exit=0, controller=True),
            dict(name="source_refusal", model=scalar["model"].replace("::core::primitive::i64", "u64"),
                 expected_exit=2, controller=False)]


def package(model):
    return dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                files={"main.xml": base64.b64encode(model.encode()).decode()})


def execute(path):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3,
            "fresh host receipt and owned output space")
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
                  host_probe_source=HOST.read_text(), worker_source=WORKER.read_text(), preparation_steps=[],
                  steps=[], models=[], consumers=[], inputs_before=execution_inputs(), references_before=references(),
                  native_inputs_before=native_inputs())
    owned = None

    def run(label, command, *, env=None, input_text=None, expected=0):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=input_text, text=True,
                           capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=list(map(str, command)), input=input_text,
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode == expected,
                label + " failed; inspect raw host receipt")
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_host_assembly_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capability_path, classpath = prepare(owned, record)
                config = record["capabilities"]
                config["schema"] = "teaql.rust_configuration.job.v4"
                run("host_probe_javac", ["javac", "--release", "17", "-cp", classpath,
                    "-d", owned / "cpp_classes", HOST])
                config["artifacts"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                capability_path.write_text(json.dumps(config, sort_keys=True))
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                    CARGO_TARGET_DIR=str(owned / "consumer_target"),
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                    TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1",
                    TEAQL_BUSINESS_ID_KEY_VERSION="1", TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
                record["environment_overrides"] = {key: env[key] for key in (
                    "CARGO_INCREMENTAL", "CARGO_PROFILE_DEV_DEBUG", "CARGO_TARGET_DIR",
                    "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER", "TEAQL_LOG_ENDPOINT",
                    "TEAQL_BUSINESS_ID_KEY_VERSION", "TEAQL_BUSINESS_ID_KEY_V1_HEX")}
                for case in models():
                    name = case["name"]
                    current = owned / name
                    current.mkdir()
                    source, cpp, report = current / "model.json", current / "cpp", current / "job.json.gz"
                    data = package(case["model"])
                    source.write_text(json.dumps(data, sort_keys=True))
                    row = dict(case, source_package=data)
                    record["models"].append(row)
                    run(name + "_host", [config["cpp_generator"]["command"][0], "-cp", classpath,
                        "CppCompilerHostProbe", source, cpp, Path(sys.executable).resolve(), WORKER,
                        capability_path, owned / "service_jobs", report,
                        *(["--controller"] if case["controller"] else [])], env=env, expected=case["expected_exit"])
                    row.update(job=read(report), sdk_exists=cpp.exists())
                    require(row["job"]["outcome"] == ("compiler_refusal" if case["expected_exit"] else "resolved")
                            and row["sdk_exists"] is (not bool(case["expected_exit"])), "actual host job outcome")
                    if case["expected_exit"]:
                        continue
                    raw = base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True)
                    require(json.loads(raw) == data, "same complete controller source package")
                    source.write_bytes(raw)
                    run(name + "_original_generate", [*config["reference_generator"]["command"], source,
                        current / "original", current / "inventory.json", "--source-package", "--semantic-storage"], env=env)
                    row.update(inventory=json.loads((current / "inventory.json").read_bytes()),
                        original_sources=hashes(current / "original"), cpp_sources=hashes(cpp),
                        provenance=json.loads((cpp / "RUST_CONFIGURATION.json").read_bytes()))
                    require(row["original_sources"] == row["job"]["original_sources_before"],
                            "independent complete original SDK matches compiler job")

                scalar = record["models"][0]
                current = owned / "scalar"
                harness = current / "harness"
                harness.mkdir()
                text = manifest(current / "original", "domain", PARITY / "compiler_scalar_storage_probe.rs")
                (harness / "Cargo.toml").write_text(text)
                shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                run("scalar_original_resolve", ["cargo", "metadata", "--offline", "--format-version", "1",
                    "--manifest-path", harness / "Cargo.toml"], env=env)
                require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"),
                        "actual reference dependency identities")
                run("scalar_original_build", ["cargo", "build", "--offline", "--locked", "--manifest-path",
                    harness / "Cargo.toml", "-j", "2"], env=env)
                command = native_command(current / "cpp/include", PARITY / "compiler_scalar_storage_probe.cpp", owned / "scalar_native")
                command[1:1] = ["-I" + str(PARITY), "-DTEAQL_EXPECTED_VALUE_TYPE=std::int64_t"]
                run("scalar_native_build", command, env=env)
                case = next(row for row in scalar_cases() if row["name"] == "qualified_integer")
                data = "".join(json.dumps(row) + "\n" for row in scalar_inputs(case))
                scalar["original"] = json_lines(run("scalar_original_run",
                    [owned / "consumer_target/debug/compiler_generated_business_probe"], env=env, input_text=data))
                scalar["cpp"] = json_lines(run("scalar_native_run", [owned / "scalar_native"], env=env, input_text=data))
                scalar.update(consumer_manifest=text, consumer_lock=(harness / "Cargo.lock").read_text())
                require(len(scalar["original"]) == len(scalar["cpp"]) == 2 and same(scalar["original"], scalar["cpp"]),
                        "complete typed SQLite/getter/E/JSON/SQL host outcomes")

                current = owned / "business_ids"
                harness = owned / "consumer"
                harness.mkdir()
                copied = owned / "rust_field_layout/src/main.rs"
                copied.parent.mkdir(parents=True)
                copied.write_bytes((PARITY / "rust_field_layout/src/main.rs").read_bytes())
                text = business.consumer_manifest(owned, record["models"][1]["inventory"])
                (harness / "Cargo.toml").write_text(text)
                shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                calls = business.commands(record)
                run("business_original_resolve", calls["original_consumer_resolve"], env=env)
                require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"),
                        "actual Business ID reference dependency identities")
                run("business_original_build", calls["original_consumers_build"], env=env)
                record.update(business_consumer_manifest=text, business_consumer_lock=(harness / "Cargo.lock").read_text(),
                              copied_original_observer=copied.read_text())
                for name in business.SOURCES:
                    source, edit = business.adapted_source(name, owned)
                    (owned / (name + ".cpp")).write_text(source)
                    row = dict(name=name, cpp_source=business.SOURCES[name][0].read_text(),
                               adapted_source=source, include_edit=edit)
                    record["consumers"].append(row)
                    run(name + "_native_build", calls[name + "_native_build"], env=env)
                    if name in business.CORPORA:
                        data = "".join(json.dumps(value["input"], ensure_ascii=True) + "\n" for value in business.cases(name))
                        row["original"] = json_lines(run(name + "_original_run", calls[name + "_original_run"], env=env, input_text=data))
                        row["cpp"] = json_lines(run(name + "_native_run", calls[name + "_native_run"], env=env, input_text=data))
                        business.compare(name, row["original"], row["cpp"])
                    else:
                        row["output"] = run(name + "_native_run", calls[name + "_native_run"], env=env)
                record["artifacts_after"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                record["service_jobs_remaining"] = list(map(str, (owned / "service_jobs").iterdir()))
                record["compiler_jobs_remaining"] = list(map(str, (owned / "compiler_jobs").iterdir()))
                record["complete"] = True
            finally:
                for row in record["models"]:
                    current = owned / row["name"]
                    row.update(original_sources_after=hashes(current / "original"), cpp_sources_after=hashes(current / "cpp"))
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(), inputs_after=execution_inputs(),
                      references_after=references(), native_inputs_after=native_inputs())
        write(path, record)
    require(record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"]
            and record["native_inputs_before"] == record["native_inputs_after"], "stable complete host execution inputs")
    require(record["service_jobs_remaining"] == record["compiler_jobs_remaining"] == [], "all host/compiler jobs cleaned")
    print("Complete host compiler and business consumers passed; owned outputs removed", flush=True)


def validate(record):
    from compiler_host_source_admission_5_0_6 import bound, frame_matches, registered_epoch_matches
    from verify_compiler_model_configuration_5_0_6 import worker_module
    require(record["complete"] is True and record["whole_goal_complete"] is False, "bounded completed host execution")
    require(registered_epoch_matches(Path(__file__), sha(record["driver_source"].encode()))
            and record["host_probe_source"] == HOST.read_text() and record["worker_source"] == WORKER.read_text(),
            "exact host execution, probe and worker sources")
    documents = bound("documents")
    executed = bound("runtime")
    before, after = record["inputs_before"], record["inputs_after"]
    changes = {name for name in before.keys() | after.keys() if before.get(name) != after.get(name)}
    require(before.keys() == after.keys() and set(after) in (set(executed["inputs_after"]), set(execution_inputs())),
            "complete captured execution input membership")
    require(not changes or changes == set(documents["paths"]) and all(before[name] == row["before_sha256"]
            and after[name] == row["after_sha256"] for name, row in documents["paths"].items()),
            "only independently qualified host documentation changes during original execution")
    require(frame_matches(execution_inputs(), after)
            and record["references_before"] == record["references_after"] == references()
            and record["native_inputs_before"] == record["native_inputs_after"] == native_inputs(),
            "current source epochs and exact reference/native identities")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and record["service_jobs_remaining"] == record["compiler_jobs_remaining"] == [], "complete owned job cleanup")
    require([row["label"] for row in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0 for row in record["preparation_steps"]),
            "actual independent Java preparation")
    require(all(registered_epoch_matches(Path(name), digest) for name, digest in record["prepared_java_sources"].items()),
            "all prepared Java sources remain qualified")
    config = record["capabilities"]
    actual = bound("runtime")
    require(config == actual["capabilities"] and config["schema"] == "teaql.rust_configuration.job.v4"
            and config["artifacts"] == record["artifacts_after"]
            and record["environment_overrides"] == actual["environment_overrides"], "exact semantic capability, linker, artifacts and process settings")
    require([row["name"] for row in record["models"]] == [row["name"] for row in models()], "complete actual host model matrix")
    owned = Path(record["owned_directory"])
    base = config["cpp_generator"]["command"]
    classpath = base[base.index("-cp") + 1]
    expected = {"host_probe_javac": ["javac", "--release", "17", "-cp", classpath,
                                     "-d", str(owned / "cpp_classes"), str(HOST)]}
    worker = worker_module()
    for row, case in zip(record["models"], models(), strict=True):
        require(all(row[key] == value for key, value in case.items())
                and row["source_package"] == package(case["model"]), "exact model bytes and host entry")
        name, job = row["name"], row["job"]
        current = owned / name
        expected[name + "_host"] = [base[0], "-cp", classpath, "CppCompilerHostProbe", str(current / "model.json"),
            str(current / "cpp"), str(Path(sys.executable).resolve()), str(WORKER), str(owned / "capabilities.json"),
            str(owned / "service_jobs"), str(current / "job.json.gz"), *(["--controller"] if case["controller"] else [])]
        require(job["complete"] is True and job["owned_directory_removed"] is True
                and job["publication_directory_removed"] is True
                and not Path(job["owned_directory"]).exists() and not Path(job["publication_directory"]).exists()
                and job["worker_sha256"] == sha(record["worker_source"].encode())
                and job["capabilities_sha256"] == sha(json.dumps(config, sort_keys=True).encode()), "actual completed host/compiler identity and cleanup")
        raw = base64.b64decode(job["source_package_bytes_base64"], validate=True)
        require(json.loads(raw) == row["source_package"] and sha(raw) == job["model_sha256"], "actual bound source envelope")
        steps = {step["label"]: step for step in job["steps"]}
        if case["expected_exit"]:
            require(job["outcome"] == "compiler_refusal" and row["sdk_exists"] is False and job["compiler_errors"]
                    and type(steps["original_compile"]["exit_code"]) is int
                    and steps["original_compile"]["exit_code"] == 101, "original refusal returns no SDK or interpretation fallback")
            continue
        require(job["outcome"] == "resolved" and row["sdk_exists"] is True, "actual complete generated host SDK")
        expected[name + "_original_generate"] = [*config["reference_generator"]["command"], str(current / "model.json"),
            str(current / "original"), str(current / "inventory.json"), "--source-package", "--semantic-storage"]
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        worker.validate_semantic_storage(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
        require(row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"]
                and row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"]
                and row["inventory"] == job["original_inventory"]
                and row["provenance"]["configuration_resolution_sha256"] == sha(json.dumps(job["resolution_document"], sort_keys=True).encode()),
                "independent original and consumed C++ SDKs match the complete compiler manifests")
    scalar = record["models"][0]
    harness = owned / "scalar/harness"
    expected["scalar_original_resolve"] = ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", str(harness / "Cargo.toml")]
    expected["scalar_original_build"] = ["cargo", "build", "--offline", "--locked", "--manifest-path", str(harness / "Cargo.toml"), "-j", "2"]
    command = native_command(owned / "scalar/cpp/include", PARITY / "compiler_scalar_storage_probe.cpp", owned / "scalar_native")
    command[1:1] = ["-I" + str(PARITY), "-DTEAQL_EXPECTED_VALUE_TYPE=std::int64_t"]
    expected["scalar_native_build"] = list(map(str, command))
    expected["scalar_original_run"] = [str(owned / "consumer_target/debug/compiler_generated_business_probe")]
    expected["scalar_native_run"] = [str(owned / "scalar_native")]
    require(scalar["consumer_manifest"] == manifest(owned / "scalar/original", "domain", PARITY / "compiler_scalar_storage_probe.rs", scalar["inventory"]["package"])
            and scalar["consumer_lock"] == actual["models"][0]["consumer_lock"], "complete actual original scalar consumer and dependency lock")
    calls = business.commands(record)
    expected["business_original_resolve"] = calls["original_consumer_resolve"]
    expected["business_original_build"] = calls["original_consumers_build"]
    require(record["business_consumer_manifest"] == business.consumer_manifest(owned, record["models"][1]["inventory"])
            and record["business_consumer_lock"] == actual["business_consumer_lock"]
            and record["copied_original_observer"] == (PARITY / "rust_field_layout/src/main.rs").read_text(),
            "complete actual original Business ID consumers and dependency lock")
    require([row["name"] for row in record["consumers"]] == list(business.SOURCES), "complete business consumer membership")
    for row in record["consumers"]:
        name = row["name"]
        source, edit = business.adapted_source(name, owned)
        require(row["cpp_source"] == business.SOURCES[name][0].read_text() and row["adapted_source"] == source
                and row["include_edit"] == edit, "unchanged business body with only its generated include redirected")
        expected[name + "_native_build"] = calls[name + "_native_build"]
        if name in business.CORPORA:
            expected[name + "_original_run"] = calls[name + "_original_run"]
            expected[name + "_native_run"] = calls[name + "_native_run"]
            business.compare(name, row["original"], row["cpp"])
        else:
            expected[name + "_native_run"] = calls[name + "_native_run"]
    require([step["label"] for step in record["steps"]] == list(expected), "every actual host/build/execution command in order")
    for step in record["steps"]:
        require(step["command"] == list(map(str, expected[step["label"]])) and type(step["exit_code"]) is int
                and step["exit_code"] == (2 if step["label"] == "source_refusal_host" else 0), "exact successful invocation or original refusal")
    steps = {step["label"]: step for step in record["steps"]}
    case = next(row for row in scalar_cases() if row["name"] == "qualified_integer")
    data = "".join(json.dumps(row) + "\n" for row in scalar_inputs(case))
    require(steps["scalar_original_run"]["input"] == steps["scalar_native_run"]["input"] == data
            and same(json_lines(steps["scalar_original_run"]["stdout"]), scalar["original"])
            and same(json_lines(steps["scalar_native_run"]["stdout"]), scalar["cpp"])
            and len(scalar["original"]) == len(scalar["cpp"]) == 2 and same(scalar["original"], scalar["cpp"])
            and [row["getter"] for row in scalar["original"][0]["rows"]] == case["values"]
            and scalar["original"][1]["rows"] == [], "actual nonempty/empty typed scalar query, E, JSON and SQL results")
    for row in record["consumers"]:
        name = row["name"]
        if name in business.CORPORA:
            data = "".join(json.dumps(value["input"], ensure_ascii=True) + "\n" for value in business.cases(name))
            require(steps[name + "_original_run"]["input"] == steps[name + "_native_run"]["input"] == data
                    and same(json_lines(steps[name + "_original_run"]["stdout"]), row["original"])
                    and same(json_lines(steps[name + "_native_run"]["stdout"]), row["cpp"]), "actual complete Business ID raw inputs and outputs")
        else:
            require(steps[name + "_native_run"]["stdout"] == row["output"], "actual native allocation and governed Save execution output")


def controls(record):
    validate(record)
    mutations = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("whole_goal", lambda r: r.update(whole_goal_complete=True)),
        ("empty_frames", lambda r: r.update(inputs_before={}, inputs_after={})),
        ("lost_model", lambda r: r["models"].pop()),
        ("changed_controller", lambda r: r["models"][0].update(controller=False)),
        ("changed_model", lambda r: r["models"][0].update(model="forged")),
        ("changed_package", lambda r: r["models"][0]["source_package"].update(files={})),
        ("lost_sdk_file", lambda r: r["models"][0]["cpp_sources"].pop(next(iter(r["models"][0]["cpp_sources"])))),
        ("hidden_refusal", lambda r: r["models"][2]["job"].update(outcome="resolved")),
        ("old_capability", lambda r: r["capabilities"].update(schema="teaql.rust_configuration.job.v2")),
        ("substituted_linker", lambda r: r["capabilities"]["linker"].update(path="/bin/true")),
        ("empty_lock", lambda r: r["models"][0].update(consumer_lock="version = 4\npackage = []\n")),
        ("forged_business_lock", lambda r: r.update(business_consumer_lock="version = 4\npackage = []\n")),
        ("changed_native", lambda r: r["native_inputs_after"].update(forged="0" * 64)),
        ("boolean_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("substituted_command", lambda r: r["steps"][1].update(command=["true"])),
        ("lost_consumer", lambda r: r["consumers"].pop()),
        ("lost_observations", lambda r: r["consumers"][0]["original"].pop()),
        ("changed_observer", lambda r: r["consumers"][0].update(adapted_source="forged")),
        ("raw_output", lambda r: r["steps"][-1].update(stdout="forged")),
        ("cleanup", lambda r: r.update(owned_directory_removed=False)),
    )
    result = []
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted host receipt corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--record", type=Path)
    mode.add_argument("--check", type=Path)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.check:
        record = read(args.check)
        validate(record)
        if args.controls:
            require(not args.controls.exists(), "preserve previous host controls")
            result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls(record))
            write(args.controls, result)
            print("Host receipt verified; rejected", len(result["controls"]), "controls; optimized=", sys.flags.optimize)
        else:
            print("Verified actual compiler host SDKs and all business outcomes")
    else:
        execute(args.record)


if __name__ == "__main__":
    main()
