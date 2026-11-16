#!/usr/bin/env python3
"""Observe required/optional temporal and JSON storage through the ordinary v4 host."""

import argparse
import ast
import base64
import copy
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_generated_business_5_0_6 import (
    execution_inputs, manifest, native_inputs, read, references, require, run_owned, sha, write,
)
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_host_assembly_5_0_6 import HOST, WORKER, package
from verify_compiler_model_configuration_5_0_6 import worker_module
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_source_service_5_0_6 import artifact_sha

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "tests/parity/compiler_scalar_storage_probe.cpp"
RUST = ROOT / "tests/parity/compiler_scalar_storage_probe.rs"
INITIAL = ROOT / "docs/parity/compiler_host_temporal_json_5_0_6.json.gz"
INITIAL_SHA = "cf9011426cb1d4e2bd3b406ef7e5517f88e8e10c4e0dfda61eddd7da4ca8990d"
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same


def cases():
    result = []
    for kind, expression, carrier, rust_type, data_type, values, sql_values in (
        ("date", "date", "Date", "chrono::naive::date::NaiveDate", "Date",
         ["2000-02-29", "1969-12-31", "2024-02-29"],
         ["'2000-02-29'", "'1969-12-31'", "'2024-02-29'"]),
        ("timestamp", "dateTime", "Timestamp", "teaql_core::time::Timestamp", "Timestamp",
         [0, -1, 1787110200123], ["0", "-1", "1787110200123"]),
        ("json", "jsonMe", "Json", "serde_json::value::Value", "LargeText",
         [{"probe": "edge", "nested": [1, None]}, "quoted"],
         ["'{\"probe\":\"edge\",\"nested\":[1,null]}'", "'\"quoted\"'"]),
    ):
        for optional in (False, True):
            name = kind + ("_optional" if optional else "_required")
            definition = expression + ("(optional=false)" if kind == "json" and not optional else "()")
            model = ('<root name="compiled_scalar_storage" org="teaql" data_service="sqlite">'
                     '<gateway name="string()"/>'
                     '<reading gateway="gateway(contextRoot=true)" value="'
                     + definition + ('?' if optional else '') + '"/></root>')
            result.append(dict(name=name, kind=kind, optional=optional, model=model,
                carrier="std::optional<" + carrier + ">" if optional else carrier,
                rust_type="core::option::Option<" + rust_type + ">" if optional else rust_type,
                data_type=data_type, values=values, sql_values=sql_values))
    result.append(dict(name="decimal_optional", kind="decimal", optional=True,
        model='<root name="compiled_scalar_storage" org="teaql" data_service="sqlite"><gateway name="string()"/><reading gateway="gateway(contextRoot=true)" value="number()?"/></root>',
        carrier="std::optional<Decimal>", rust_type="core::option::Option<rust_decimal::decimal::Decimal>",
        data_type="Decimal", values=["123.45", "-9.125"], sql_values=["'123.45'", "'-9.125'"]))
    return result


def selected_cases(selection):
    if selection is None:
        return cases()
    require(type(selection) is list and selection and len(set(selection)) == len(selection)
            and set(selection) <= {case["name"] for case in cases()}, "explicit unique supported selection")
    return [case for name in selection for case in cases() if case["name"] == name]


def datasets(case):
    schema = ("CREATE TABLE gateway_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);"
              "CREATE TABLE reading_data(id INTEGER PRIMARY KEY,value,gateway INTEGER,version INTEGER);"
              "INSERT INTO gateway_data VALUES(1,'edge',1);")
    if case["kind"] in ("json", "decimal"):
        declaration = "JSON" if case["kind"] == "json" else "DECIMAL"
        schema = schema.replace("value,gateway", "value " + declaration + ",gateway")
    values, sql_values = list(case["values"]), list(case["sql_values"])
    if case["optional"]:
        values.append(None)
        sql_values.append("NULL")
    populated = schema + "INSERT INTO reading_data VALUES" + ",".join(
        "(" + str(index + 7) + "," + value + ",1,1)" for index, value in enumerate(sql_values)) + ";"
    result = [dict(name="values", input=dict(sql=populated), expected_values=values),
              dict(name="empty", input=dict(sql=schema), expected_values=[])]
    if case["kind"] == "json":
        result.insert(0, dict(name="null", input=dict(sql=schema + "INSERT INTO reading_data VALUES(7,NULL,1,1);"),
                              expected_values=[None]))
    return result


def commands(case, owned, config):
    current = owned / case["name"]
    base = config["cpp_generator"]["command"]
    classpath = base[base.index("-cp") + 1]
    native = native_command(current / "cpp/include", CPP, owned / "native_probe")
    native[1:1] = ["-I" + str(ROOT / "tests/parity"), "-DTEAQL_EXPECTED_VALUE_TYPE=" + case["carrier"]]
    return dict(
        host=[base[0], "-cp", classpath, "CppCompilerHostProbe", str(current / "model.json"),
              str(current / "cpp"), str(Path(sys.executable).resolve()), str(WORKER),
              str(owned / "capabilities.json"), str(owned / "service_jobs"), str(current / "job.json.gz"), "--controller"],
        original_generate=[*config["reference_generator"]["command"], str(current / "model.json"),
                           str(current / "original"), str(current / "inventory.json"), "--source-package", "--semantic-storage"],
        original_resolve=["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", str(current / "harness/Cargo.toml")],
        original_build=["cargo", "build", "--offline", "--locked", "--manifest-path", str(current / "harness/Cargo.toml"), "-j", "2"],
        native_build=native,
        original_run=[str(owned / "consumer_target/debug/compiler_generated_business_probe")],
        native_run=[str(owned / "native_probe")],
    )


def execute(path, selection=None):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh temporal/JSON receipt and owned output space")
    selected = selected_cases(selection)
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
        cpp_source=CPP.read_text(), rust_source=RUST.read_text(), host_source=HOST.read_text(),
        worker_source=WORKER.read_text(), steps=[], preparation_steps=[], cases=[],
        inputs_before=execution_inputs(), references_before=references(), native_before=native_inputs(),
        selected_cases=[case["name"] for case in selected])
    owned = None

    def run(label, command, *, env=None, payload=None, expected=(0,)):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=list(map(str, command)), input=payload,
            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode in expected, "unexpected process outcome: " + label)
        return result

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_host_temporal_json_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capability, classpath = prepare(owned, record)
                config = record["capabilities"]
                config["schema"] = "teaql.rust_configuration.job.v4"
                run("host_javac", ["javac", "--release", "17", "-cp", classpath, "-d", str(owned / "cpp_classes"), str(HOST)])
                config["artifacts"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                capability.write_text(json.dumps(config, sort_keys=True))
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                    CARGO_TARGET_DIR=str(owned / "consumer_target"),
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                    TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                record["environment_overrides"] = {key: env[key] for key in (
                    "CARGO_INCREMENTAL", "CARGO_PROFILE_DEV_DEBUG", "CARGO_TARGET_DIR",
                    "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER", "TEAQL_LOG_ENDPOINT")}
                for case in selected:
                    name = case["name"]
                    current = owned / name
                    current.mkdir()
                    source = current / "model.json"
                    source.write_text(json.dumps(package(case["model"]), sort_keys=True))
                    row = dict(case, source_package=package(case["model"]))
                    record["cases"].append(row)
                    calls = commands(case, owned, config)
                    result = run(name + "_host", calls["host"], env=env, expected=(0, 2))
                    row.update(host_exit_code=result.returncode, job=read(current / "job.json.gz"),
                               sdk_exists=(current / "cpp").exists())
                    if result.returncode:
                        continue
                    raw = base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True)
                    require(json.loads(raw) == row["source_package"], "controller source envelope preserves all model bytes")
                    source.write_bytes(raw)
                    run(name + "_original_generate", calls["original_generate"], env=env)
                    row.update(original_sources=hashes(current / "original"), cpp_sources=hashes(current / "cpp"),
                        provenance=json.loads((current / "cpp/RUST_CONFIGURATION.json").read_bytes()),
                        inventory=json.loads((current / "inventory.json").read_bytes()))
                    require(row["original_sources"] == row["job"]["original_sources_before"], "same complete original SDK")
                    harness = current / "harness"
                    harness.mkdir()
                    text = manifest(current / "original", "domain", RUST)
                    (harness / "Cargo.toml").write_text(text)
                    shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                    run(name + "_original_resolve", calls["original_resolve"], env=env)
                    require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "exact locked reference dependencies")
                    run(name + "_original_build", calls["original_build"], env=env)
                    run(name + "_native_build", calls["native_build"], env=env)
                    row.update(consumer_manifest=text, consumer_lock=(harness / "Cargo.lock").read_text(), observations=[])
                    for data in datasets(case):
                        payload = json.dumps(data["input"]) + "\n"
                        prefix = name + "_" + data["name"]
                        original = run(prefix + "_original_run", calls["original_run"], env=env, payload=payload,
                                       expected=(0, 101) if case["kind"] == "json" else (0,))
                        native = run(prefix + "_native_run", calls["native_run"], env=env, payload=payload)
                        row["observations"].append(dict(data, original_exit_code=original.returncode,
                            original=json_lines(original.stdout) if original.returncode == 0 else None,
                            cpp=json_lines(native.stdout)))
                    row.update(original_sources_after=hashes(current / "original"), cpp_sources_after=hashes(current / "cpp"))
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(),
            inputs_after=execution_inputs(), references_after=references(), native_after=native_inputs())
        write(path, record)
    require(record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"]
            and record["native_before"] == record["native_after"], "frozen execution source, references and native inputs")
    print("Complete temporal/JSON collection; run --check for semantic qualification", flush=True)


def validate(record, prefix=False):
    source = Path(__file__).read_text()
    current = execution_inputs()
    if prefix:
        require(sha(INITIAL.read_bytes()) == INITIAL_SHA and record == read(INITIAL), "immutable initial failure and completed temporal prefix")
        before = {node.name: node for node in ast.parse(record["driver_source"]).body if isinstance(node, ast.FunctionDef)}
        after = {node.name: node for node in ast.parse(source).body if isinstance(node, ast.FunctionDef)}
        require(ast.dump(before["commands"]) == ast.dump(after["commands"]), "unchanged command body")
        restored_data = copy.deepcopy(after["datasets"])
        declaration = ast.parse('if case["kind"] in ("json", "decimal"):\n    declaration = "JSON" if case["kind"] == "json" else "DECIMAL"\n    schema = schema.replace("value,gateway", "value " + declaration + ",gateway")').body[0]
        changes = [node for node in restored_data.body if ast.dump(node) == ast.dump(declaration)]
        require(len(changes) == 1, "one explicit JSON/Decimal fixture declaration")
        restored_data.body = [node for node in restored_data.body if ast.dump(node) != ast.dump(declaration)]
        require(ast.dump(restored_data) == ast.dump(before["datasets"]), "temporal inputs unchanged after removing JSON/Decimal declarations only")
        restored = copy.deepcopy(after["execute"])
        restored.args = before["execute"].args
        restored.body = [node for node in restored.body if not (isinstance(node, ast.Assign)
            and any(isinstance(target, ast.Name) and target.id == "selected" for target in node.targets))]
        for node in ast.walk(restored):
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "dict":
                node.keywords = [value for value in node.keywords if value.arg != "selected_cases"]
            if isinstance(node, ast.For) and isinstance(node.iter, ast.Name) and node.iter.id == "selected":
                node.iter = ast.Call(func=ast.Name(id="cases", ctx=ast.Load()), args=[], keywords=[])
        require(ast.dump(restored) == ast.dump(before["execute"]), "exact preceding execution body after removing selection only")
        key = "runtime:scripts/verify_compiler_host_temporal_json_5_0_6.py"
        require(record["inputs_before"] == record["inputs_after"] and current.keys() == record["inputs_after"].keys()
                and all(value == current[name] or name == key and value == sha(record["driver_source"].encode())
                        and current[name] == sha(source.encode()) for name, value in record["inputs_after"].items()), "only the reviewed collector source changes")
        require(record["complete"] is False and len(record["cases"]) == 5
                and record["steps"][-1]["label"] == "json_required_native_build" and record["steps"][-1]["exit_code"] == 1
                and "std::optional<teaql::Json>, teaql::Json" in record["steps"][-1]["stderr"], "preserved JSON optionality probe failure")
        expected = cases()[:4]
        rows = record["cases"][:4]
        selected_steps = [step for step in record["steps"] if step["label"] == "host_javac"
            or any(step["label"].startswith(case["name"] + "_") for case in expected)]
    else:
        require(record["complete"] is True and record["driver_source"] == source
                and record["inputs_before"] == record["inputs_after"] == current, "exact complete collector and frozen source frame")
        expected = selected_cases(record["selected_cases"])
        rows = record["cases"]
        selected_steps = record["steps"]
    require(record["whole_goal_complete"] is False
            and record["cpp_source"] == CPP.read_text() and record["rust_source"] == RUST.read_text()
            and record["host_source"] == HOST.read_text() and record["worker_source"] == WORKER.read_text(), "exact complete collector and unchanged observers")
    require(record["references_before"] == record["references_after"] == references()
            and record["native_before"] == record["native_after"] == native_inputs(), "complete unchanged input frames")
    owned = Path(record["owned_directory"])
    require(record["owned_directory_removed"] is True and not owned.exists(), "owned outputs reclaimed")
    require(record["capabilities"]["schema"] == "teaql.rust_configuration.job.v4"
            and [step["label"] for step in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["preparation_steps"]), "explicit v4 and successful independent preparation")
    require([row["name"] for row in rows] == [row["name"] for row in expected], "all selected exact configurations")
    steps = {step["label"]: step for step in selected_steps}
    require(len(steps) == len(selected_steps), "unique actual process labels")
    labels = ["host_javac"]
    base = record["capabilities"]["cpp_generator"]["command"]
    require(steps["host_javac"]["command"] == ["javac", "--release", "17", "-cp", base[base.index("-cp") + 1],
            "-d", str(owned / "cpp_classes"), str(HOST)]
            and type(steps["host_javac"]["exit_code"]) is int and steps["host_javac"]["exit_code"] == 0,
            "actual successful host compilation")
    worker = worker_module()
    for row, case in zip(rows, expected, strict=True):
        require(all(row[key] == value for key, value in case.items()), "exact type, optionality, model and values")
        name, job = row["name"], row["job"]
        calls = commands(case, owned, record["capabilities"])
        labels.append(name + "_host")
        require(steps[name + "_host"]["command"] == calls["host"] and row["source_package"] == package(case["model"])
                and json.loads(base64.b64decode(job["source_package_bytes_base64"], validate=True)) == row["source_package"]
                and job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and not Path(job["owned_directory"]).exists() and not Path(job["publication_directory"]).exists(), "actual host source and job cleanup")
        require(job["complete"] is True and job["worker_sha256"] == sha(record["worker_source"].encode())
                and job["capabilities_sha256"] == sha(json.dumps(record["capabilities"], sort_keys=True).encode()), "actual worker and bound capability")
        if row["host_exit_code"]:
            require(type(row["host_exit_code"]) is int and row["host_exit_code"] == steps[name + "_host"]["exit_code"] == 2
                    and job["outcome"] == "compiler_refusal" and job["compiler_errors"] and row["sdk_exists"] is False, "explicit original whole-library refusal")
            continue
        require(type(row["host_exit_code"]) is int and steps[name + "_host"]["exit_code"] == row["host_exit_code"] == 0
                and job["outcome"] == "resolved" and row["sdk_exists"] is True,
                "complete original/native SDKs")
        worker.validate_configuration(job["original_inventory"]["entities"], job["descriptors"], job["configuration"])
        worker.validate_semantic_storage(job["original_inventory"]["entities"], job["configuration"], job["scalar_types"])
        require(job["scalar_observer_source"] == worker.semantic_storage_source(job["original_inventory"]["entities"])
                and job["scalar_artifacts"] == job["configuration_artifacts"], "same-library TypeId/storage observation")
        job_steps = {step["label"]: step for step in job["steps"]}
        require(all(label in job_steps and type(job_steps[label]["exit_code"]) is int and job_steps[label]["exit_code"] == 0
                    for label in ("original_compile", "scalar_observer_compile", "original_scalar_types", "cpp_generate"))
                and json.loads(job_steps["original_scalar_types"]["stdout"]) == job["scalar_types"], "actual original compilation, scalar observation and C++ generation")
        scalars = [prop for entity in job["scalar_types"] for prop in entity["properties"] if prop["name"] == "value"]
        properties = [prop for entity in job["configuration"] for prop in entity["properties"] if prop["name"] == "value"]
        require(len(scalars) == len(properties) == 1 and scalars[0]["rust_type"] == scalars[0]["storage_type"] == case["rust_type"]
                and scalars[0]["semantic_kind"] == "scalar" and scalars[0]["semantic_type"] is None
                and properties[0]["data_type"] == case["data_type"] and properties[0]["nullable"] is case["optional"], "exact scalar identity, descriptor and optionality")
        require(row["inventory"] == job["original_inventory"]
                and row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"]
                and row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"], "complete immutable SDK source identities")
        for suffix in ("original_generate", "original_resolve", "original_build", "native_build"):
            label = name + "_" + suffix
            labels.append(label)
            require(steps[label]["command"] == calls[suffix] and type(steps[label]["exit_code"]) is int
                    and steps[label]["exit_code"] == 0, "successful exact generation and compiler command")
        require(len(row["observations"]) == len(datasets(case)), "complete nonempty/NULL/empty dataset membership")
        for observation, data in zip(row["observations"], datasets(case), strict=True):
            require(all(observation[key] == value for key, value in data.items()), "exact shared input bytes and expected values")
            prefix = name + "_" + data["name"]
            original, native = (steps[prefix + "_" + runtime + "_run"] for runtime in ("original", "native"))
            labels.extend([prefix + "_original_run", prefix + "_native_run"])
            require(original["command"] == calls["original_run"] and native["command"] == calls["native_run"]
                    and original["input"] == native["input"] == json.dumps(data["input"]) + "\n"
                    and type(native["exit_code"]) is int and native["exit_code"] == 0, "actual shared dataset and terminal native invocation")
            require(observation["cpp"] == json_lines(native["stdout"]) and len(observation["cpp"]) == 1
                    and [r["getter"] for r in observation["cpp"][0]["rows"]] == data["expected_values"]
                    and all(r["getter"] == r["expression"] == r["json"]["value"] for r in observation["cpp"][0]["rows"]), "actual typed native getter, E and JSON values")
            require(type(original["exit_code"]) is int and observation["original_exit_code"] == original["exit_code"], "actual original outcome")
            if original["exit_code"]:
                require(case["kind"] == "json" and data["name"] == "values" and original["exit_code"] == 101
                        and observation["original"] is None and "called `Result::unwrap()`" in original["stderr"]
                        and "Reading" in original["stderr"] and "value" in original["stderr"], "explicit JSON original read failure, separate from equal results")
            else:
                require(observation["original"] == json_lines(original["stdout"])
                        and same(observation["original"], observation["cpp"]), "actual original/native query, E, JSON and SQL outcomes")
    require([step["label"] for step in selected_steps] == labels
            and all(type(step["exit_code"]) is int for step in selected_steps), "complete actual terminal command membership")


def controls(record):
    validate(record)
    index = next(i for i, row in enumerate(record["cases"]) if row["job"]["outcome"] == "resolved")
    changes = (
        ("unfinished", lambda r: r.update(complete=False)),
        ("missing_case", lambda r: r["cases"].pop()),
        ("changed_type", lambda r: r["cases"][index].update(carrier="unqualified")),
        ("wrong_scalar", lambda r: r["cases"][index]["job"]["scalar_types"][0]["properties"][0].update(rust_type=False)),
        ("lost_sdk", lambda r: r["cases"][index]["cpp_sources"].pop(next(iter(r["cases"][index]["cpp_sources"])))),
        ("missing_input", lambda r: r["cases"][index]["observations"].pop()),
        ("wrong_value", lambda r: r["cases"][index]["observations"][0]["cpp"][0]["rows"][0].update(getter="wrong")),
        ("changed_command", lambda r: r["steps"][1].update(command=["true"])),
        ("changed_observer", lambda r: r.update(rust_source="fn main() {}\n")),
        ("missing_preparation", lambda r: r["preparation_steps"].clear()),
        ("missing_original_compile", lambda r: r["cases"][index]["job"].update(steps=[step for step in r["cases"][index]["job"]["steps"] if step["label"] != "original_compile"])),
        ("changed_native_command", lambda r: r["steps"][-1].update(command=["true"])),
        ("changed_input", lambda r: r["steps"][-1].update(input="{}\n")),
        ("changed_stdout", lambda r: r["steps"][-1].update(stdout="{}\n")),
        ("boolean_exit", lambda r: r["steps"][-1].update(exit_code=False)),
    )
    rows = []
    for name, change in changes:
        damaged = copy.deepcopy(record)
        change(damaged)
        try:
            validate(damaged)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            rows.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted temporal/JSON corruption: " + name)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    parser.add_argument("--case", action="append", choices=[case["name"] for case in cases()])
    args = parser.parse_args()
    if args.execute:
        execute(args.record, args.case)
    else:
        record = read(args.record)
        validate(record, prefix=args.record.resolve() == INITIAL)
        if args.controls:
            require(not args.controls.exists(), "preserve preceding controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls(record)))
        scope = "completed temporal prefix; initial JSON probe failure retained" if args.record.resolve() == INITIAL else "selected temporal/JSON execution"
        print("Verified", scope, "; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
