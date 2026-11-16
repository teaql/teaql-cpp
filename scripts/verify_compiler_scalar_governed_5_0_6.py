#!/usr/bin/env python3
"""Join qualified compiler SDKs to generated schema and governed scalar saves."""

import argparse
import base64
import copy
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_generated_business_5_0_6 import (
    execution_inputs, manifest, native_inputs, read, references, require, run_owned, sha, write,
)
from verify_compiler_governed_execution_5_0_6 import LOCK, prepare
from verify_compiler_projection_generation_5_0_6 import hashes, native_command

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "tests/parity/compiler_scalar_governed_probe.cpp"
RUST = ROOT / "tests/parity/compiler_scalar_governed_probe.rs"
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same


def selected_models(selection):
    result = []
    for name, count in (("compiler_host_temporal_json_5_0_6.json.gz", 4),
                        ("compiler_host_temporal_json_typed_storage_5_0_6.json.gz", 3)):
        path = ROOT / "docs/parity" / name
        require(path.read_bytes() == subprocess.check_output(["git", "show", "HEAD:" + str(path.relative_to(ROOT))], cwd=ROOT),
                "unchanged committed compiler receipt")
        for case in read(path)["cases"][:count]:
            require(case["job"]["outcome"] == "resolved" and case["cpp_sources"] == case["job"]["cpp_sources"], "qualified positive complete SDK")
            result.append(dict(case=case, compiler_receipt=str(path.relative_to(ROOT)), compiler_receipt_sha256=sha(path.read_bytes())))
    if selection:
        require(len(set(selection)) == len(selection) and set(selection) <= {row["case"]["name"] for row in result}, "unique known model selection")
        return [row for name in selection for row in result if row["case"]["name"] == name]
    return result


def typed(case, value):
    if value is None and case["optional"]:
        return dict(kind="null")
    kind = case["kind"]
    return dict(kind=kind, value=str(value) if kind == "timestamp" else value)


def inputs(case):
    values = case["values"]
    result = []
    for mode in ("create", "update", "graph", "deny", "approved", "fail_root", "fail_child",
                 "stale_root", "stale_child", "sink_fail"):
        result.append(dict(mode=mode, value=values[0], next_value=values[1],
                           typed=typed(case, values[0]), next_typed=typed(case, values[1])))
    if case["optional"]:
        for mode in ("create", "update", "graph"):
            first = None if mode == "create" else values[0]
            result.append(dict(mode=mode, scenario=mode + "_null", value=first, next_value=None,
                               typed=typed(case, first), next_typed=typed(case, None)))
    return result


def execute(path, selection):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh governed scalar receipt and owned output space")
    models = selected_models(selection)
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
        cpp_source=CPP.read_text(), rust_source=RUST.read_text(), preparation_steps=[], steps=[], models=[],
        selected_models=[row["case"]["name"] for row in models], inputs_before=execution_inputs(),
        references_before=references(), native_before=native_inputs(), compiler_jobs_reused=True)
    owned = None

    def run(label, command, *, env=None, payload=None):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=list(map(str, command)), input=payload,
            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode == 0, "actual governed consumer command failed: " + label)
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_scalar_governed_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                _, classpath = prepare(owned, record)
                config = record["capabilities"]
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                    CARGO_TARGET_DIR=str(owned / "consumer_target"),
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                    TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                record["environment_overrides"] = {name: env[name] for name in (
                    "CARGO_INCREMENTAL", "CARGO_PROFILE_DEV_DEBUG", "CARGO_TARGET_DIR",
                    "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER", "TEAQL_LOG_ENDPOINT")}
                for source in models:
                    case = source["case"]
                    name = case["name"]
                    current = owned / name
                    current.mkdir()
                    model, document = current / "model.json", current / "resolution.json"
                    model.write_bytes(base64.b64decode(case["job"]["source_package_bytes_base64"], validate=True))
                    document.write_text(json.dumps(case["job"]["resolution_document"], sort_keys=True))
                    row = dict(name=name, kind=case["kind"], optional=case["optional"], carrier=case["carrier"],
                        compiler_receipt=source["compiler_receipt"], compiler_receipt_sha256=source["compiler_receipt_sha256"],
                        original_execution_reused=True, source_package_bytes_base64=case["job"]["source_package_bytes_base64"],
                        resolution_document=case["job"]["resolution_document"], inputs=inputs(case))
                    record["models"].append(row)
                    run(name + "_cpp_generate", [config["cpp_generator"]["command"][0], "-cp", classpath, "CppCompiledModel",
                        model, current / "cpp", "--rust-configuration=" + str(document), "--rust-toolchain-lock=" + str(LOCK), "--source-package"], env=env)
                    row["cpp_sources"] = hashes(current / "cpp")
                    require(row["cpp_sources"] == case["cpp_sources"], "entire regenerated C++ SDK matches qualified compiler output")
                    run(name + "_original_generate", [*config["reference_generator"]["command"], model,
                        current / "original", current / "inventory.json", "--source-package", "--semantic-storage"], env=env)
                    row["original_sources"] = hashes(current / "original")
                    require(row["original_sources"] == case["original_sources"], "entire independently regenerated original SDK matches compiler output")
                    harness = current / "harness"
                    harness.mkdir()
                    rust_source = RUST.read_text()
                    observer = RUST
                    row["rust_argument_adapter"] = None
                    if case["optional"] and case["kind"] != "timestamp":
                        before = "entity.update_value(desired);"
                        after = "entity.update_value(desired.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null));"
                        require(rust_source.count(before) == 3, "three existing optional scalar mutation arguments")
                        rust_source = rust_source.replace(before, after)
                        observer = current / "observer.rs"
                        observer.write_text(rust_source)
                        row["rust_argument_adapter"] = dict(before=before, after=after, replacements=3)
                    row["rust_observer_source"] = rust_source
                    text = manifest(current / "original", "domain", observer)
                    (harness / "Cargo.toml").write_text(text)
                    shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                    run(name + "_original_resolve", ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", harness / "Cargo.toml"], env=env)
                    require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "locked original dependency identities")
                    run(name + "_original_build", ["cargo", "build", "--offline", "--locked", "--manifest-path", harness / "Cargo.toml", "-j", "2"], env=env)
                    command = native_command(current / "cpp/include", CPP, owned / "native_probe")
                    command[1:1] = ["-I" + str(ROOT / "tests/parity"), "-DTEAQL_EXPECTED_VALUE_TYPE=" + case["carrier"]]
                    run(name + "_native_build", command, env=env)
                    payload = "".join(json.dumps(value) + "\n" for value in row["inputs"])
                    row["original"] = json_lines(run(name + "_original_run", [owned / "consumer_target/debug/compiler_generated_business_probe"], env=env, payload=payload))
                    row["cpp"] = json_lines(run(name + "_native_run", [owned / "native_probe"], env=env, payload=payload))
                    row.update(consumer_manifest=text, consumer_lock=(harness / "Cargo.lock").read_text(),
                        original_sources_after=hashes(current / "original"), cpp_sources_after=hashes(current / "cpp"))
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(), inputs_after=execution_inputs(),
            references_after=references(), native_after=native_inputs())
        write(path, record)
    require(record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"]
            and record["native_before"] == record["native_after"], "frozen governed scalar inputs")
    print("Governed scalar execution collected; comparisons remain a separate exit", flush=True)


def check_native_outcome(observation, data, case):
    mode = data["mode"]
    committed = mode in ("create", "update", "graph", "approved", "sink_fail")
    expected = dict(ok=mode in ("create", "update", "graph", "approved"),
                    policy_error=mode == "deny", conflict=mode.startswith("stale_"),
                    committed_error=mode == "sink_fail")
    require(same(observation["outcome"], expected), "actual save/denial/rollback/conflict/committed outcome")
    bootstrap = dict(gateways=[dict(id=1, name="string()", version=1)], readings=[])
    require(same(observation["bootstrap_first"], bootstrap) and same(observation["bootstrap_second"], bootstrap),
            "registered schema and generated repeatable root bootstrap")
    schema = observation["schema"]
    column = lambda name, kind, nullable=False, primary=False: dict(
        name=name, type=kind, notnull=int(not nullable), pk=int(primary), dflt_value=None)
    scalar_type = dict(date="DATE", timestamp="TIMESTAMP", json="JSON", decimal="NUMERIC(19,7)")[case["kind"]]
    require(same(schema, dict(
        gateway=[column("id", "INTEGER", primary=True), column("name", "VARCHAR(100)"), column("version", "INTEGER")],
        reading=[column("id", "INTEGER", primary=True), column("value", scalar_type, case["optional"]),
                 column("version", "INTEGER"), column("gateway", "INTEGER")],
        foreign_keys=[dict(table="gateway_data", **{"from": "gateway"}, to="id", on_update="NO ACTION", on_delete="NO ACTION")],
    )), "complete actual schema, optionality and foreign key")
    value = data["value"] if mode == "create" or not committed else data["next_value"]
    if value is not None and case["kind"] == "json":
        value = json.dumps(value, sort_keys=True, separators=(",", ":"))
    if value is not None and case["kind"] == "decimal":
        value = float(value)
    graph = mode not in ("create", "update")
    require(same(observation["database"], dict(
        gateways=[dict(id=1, name="UPDATED-ROOT" if graph and committed else "string()",
                       version=2 if graph and committed or mode == "stale_root" else 1)],
        readings=[dict(id=7, gateway=1, value=value, version=2 if mode != "create" and committed or mode == "stale_child" else 1)],
    )), "actual authoritative values and versions, including rollback and committed errors")
    fix = [dict(entity="Reading", path="gateway", source="Context", label="activeRoot:Gateway")]
    require(same(observation["fixes"], fix if mode == "create" else [])
            and same(observation["ids"], ["reading"] if mode == "create" else []), "Context Fix and generated ID calls")
    if mode != "create":
        require(same(observation["seed"], dict(ok=True, policy_error=False, conflict=False, committed_error=False))
                and same(observation["seed_fixes"], fix), "actual audited seed and Fix")
    fields = dict(value=data["value"] if mode == "create" else data["next_value"])
    if mode == "create":
        fields.update(gateway_id=1, id=7)
    operations = [dict(entity="Reading", id=7, kind="Create" if mode == "create" else "Update",
                       original_version=None if mode == "create" else 1, fields=fields)]
    if graph:
        operations.insert(0, dict(entity="Gateway", id=1, kind="Update", original_version=1, fields=dict(name="UPDATED-ROOT")))
    root = "Gateway" if graph else "Reading"
    reason = "Save typed scalar " + ("aggregate" if graph else "creation" if mode == "create" else "update")
    require(same(observation["plans"], [dict(root=root, request=root + ".saveGraph", reason=reason, operations=operations)]),
            "complete immutable whole-graph policy plan with typed changes")
    events = observation["events"]
    require(len(events) == (len(operations) if committed else 0), "post-commit audit membership; none on rollback")
    for event, operation in zip(events, operations):
        require(event["entity"] == operation["entity"] and type(event["id"]) is int and event["id"] == operation["id"]
                and event["kind"] == ("Created" if mode == "create" else "Updated")
                and event["approved"] is (mode == "approved")
                and same(event["trace"], [dict(kind="AuditReason", entity=root, id=1 if graph else 7, comment=reason)])
                and event["fields"] and all(field["masked"] is False and field["truncated"] is False
                                            and type(field["value"]) is str for field in event["fields"]),
                "safe fields, approval and complete canonical trace")
    require(observation["logs"] and all(set(log) == {"sql", "comment", "purpose", "reason", "params"}
            and type(log["sql"]) is str and log["sql"] and type(log["params"]) is list
            and (log["comment"] or log["reason"]) for log in observation["logs"]), "actual parameterized business SQL and intent")


def validate(record):
    from compiler_scalar_governed_source_admission_5_0_6 import bound, frame_matches, source_matches
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["compiler_jobs_reused"] is True, "complete bounded collection and explicit compiler reuse")
    require(record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and source_matches(Path(__file__), sha(record["driver_source"].encode()))
            and record["cpp_source"] == CPP.read_text() and record["rust_source"] == RUST.read_text(),
            "unchanged executed observers and explicitly admitted collector-only source transitions")
    require(record["references_before"] == record["references_after"] == references()
            and record["native_before"] == record["native_after"] == native_inputs(), "exact locked references and native artifacts")
    owned = Path(record["owned_directory"])
    require(record["owned_directory_removed"] is True and not owned.exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0, "actual owned-output cleanup")
    from verify_compiler_governed_execution_5_0_6 import SEED, SEED_SHA
    seed = read(SEED)
    require(record["preparation_seed_sha256"] == SEED_SHA and sha(SEED.read_bytes()) == SEED_SHA,
            "immutable independent Java preparation recipe")
    require([step["label"] for step in record["preparation_steps"]] == ["original_bridge_javac", "cpp_javac"], "complete independent preparation")
    producer = ROOT.parent / "teaql_code_gen_cpp"
    for step in record["preparation_steps"]:
        command = [part.replace(seed["owned_directory"], str(owned))
                   for old in seed["steps"] if old["label"] == step["label"] for part in old["command"]]
        if step["label"] == "cpp_javac":
            for package in ("model", "generator"):
                for path in sorted((producer / package / "src/main/java").rglob("*.java")):
                    if str(path) not in command:
                        command.append(str(path))
        require(step["command"] == command and type(step["exit_code"]) is int and step["exit_code"] == 0,
                "successful exact independent Java compilation")
    prepared = {name for step in record["preparation_steps"] for name in step["command"] if name.endswith(".java")}
    require(set(record["prepared_java_sources"]) == prepared and all(
        sha(Path(name).read_bytes()) == digest for name, digest in record["prepared_java_sources"].items()), "exact prepared Java source identities")
    config = record["capabilities"]
    canonical = bound("date" if record["selected_models"] == ["date_required"] else "remaining")
    require(config == canonical["capabilities"] and record["environment_overrides"] == canonical["environment_overrides"],
            "actual compiler capabilities and isolated consumer environment")
    selected = selected_models(record["selected_models"])
    require([row["name"] for row in record["models"]] == record["selected_models"] == canonical["selected_models"], "complete selected model membership")
    steps = {step["label"]: step for step in record["steps"]}
    require(len(steps) == len(record["steps"]), "unique actual terminal commands")
    labels = []
    base = config["cpp_generator"]["command"]
    classpath = base[base.index("-cp") + 1]
    for row, source in zip(record["models"], selected, strict=True):
        case = source["case"]
        name = case["name"]
        current = owned / name
        require(all(same(row[key], case[key]) for key in ("name", "kind", "optional", "carrier"))
                and row["compiler_receipt"] == source["compiler_receipt"]
                and row["compiler_receipt_sha256"] == source["compiler_receipt_sha256"]
                and row["original_execution_reused"] is True
                and row["source_package_bytes_base64"] == case["job"]["source_package_bytes_base64"]
                and row["resolution_document"] == case["job"]["resolution_document"], "exact compiler witness, source envelope and resolution")
        require(row["original_sources"] == row["original_sources_after"] == case["original_sources"]
                and row["cpp_sources"] == row["cpp_sources_after"] == case["cpp_sources"], "every original/C++ SDK file retained exactly")
        observer = RUST
        rust_source = RUST.read_text()
        adapter = None
        if case["optional"] and case["kind"] != "timestamp":
            before = "entity.update_value(desired);"
            after = "entity.update_value(desired.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null));"
            require(rust_source.count(before) == 3, "three scalar setter arguments")
            rust_source = rust_source.replace(before, after)
            adapter = dict(before=before, after=after, replacements=3)
            observer = current / "observer.rs"
        if "rust_argument_adapter" in row:
            require(row["rust_argument_adapter"] == adapter and row["rust_observer_source"] == rust_source, "only actual optional setter argument adaptation")
        else:
            require(name == "date_required" and adapter is None, "preceding direct required Date setter")
        native = native_command(current / "cpp/include", CPP, owned / "native_probe")
        native[1:1] = ["-I" + str(ROOT / "tests/parity"), "-DTEAQL_EXPECTED_VALUE_TYPE=" + case["carrier"]]
        calls = dict(
            cpp_generate=[base[0], "-cp", classpath, "CppCompiledModel", str(current / "model.json"), str(current / "cpp"),
                          "--rust-configuration=" + str(current / "resolution.json"), "--rust-toolchain-lock=" + str(LOCK), "--source-package"],
            original_generate=[*config["reference_generator"]["command"], str(current / "model.json"), str(current / "original"), str(current / "inventory.json"), "--source-package", "--semantic-storage"],
            original_resolve=["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", str(current / "harness/Cargo.toml")],
            original_build=["cargo", "build", "--offline", "--locked", "--manifest-path", str(current / "harness/Cargo.toml"), "-j", "2"],
            native_build=list(map(str, native)), original_run=[str(owned / "consumer_target/debug/compiler_generated_business_probe")],
            native_run=[str(owned / "native_probe")])
        require(same(row["inputs"], inputs(case)), "complete exact ten scenarios and applicable three NULL scenarios")
        payload = "".join(json.dumps(value) + "\n" for value in inputs(case))
        for suffix, command in calls.items():
            label = name + "_" + suffix
            labels.append(label)
            require(steps[label]["command"] == command and type(steps[label]["exit_code"]) is int and steps[label]["exit_code"] == 0
                    and steps[label]["input"] == (payload if suffix.endswith("_run") else None), "exact terminal generation/build/run command and input")
        package = json.loads(re.search(r'domain=\{package=("[^"]+")', case["consumer_manifest"])[1])
        require(row["consumer_manifest"] == manifest(current / "original", "domain", observer, package)
                and row["consumer_lock"] == next(value["consumer_lock"] for value in canonical["models"] if value["name"] == name),
                "actual generated-library dependency/observer binding and locked packages")
        require(same(row["original"], json_lines(steps[name + "_original_run"]["stdout"]))
                and same(row["cpp"], json_lines(steps[name + "_native_run"]["stdout"]))
                and len(row["original"]) == len(row["cpp"]) == len(row["inputs"]), "complete actual LF-framed process outputs")
        for data, observation in zip(row["inputs"], row["cpp"], strict=True):
            check_native_outcome(observation, data, case)
        if case["kind"] == "json":
            captured = next(value for value in canonical["models"] if value["name"] == name)
            require(same(row["original"], captured["original"]) and same(row["cpp"], captured["cpp"])
                    and "invalid field value: Text" in steps[name + "_original_run"]["stderr"],
                    "exact approved JSON metadata exception; no generalized mismatch suppression")
        else:
            require(same(row["original"], row["cpp"]), "complete shared schema/Fix/graph/Policy/audit/SQL outcomes")
    require([step["label"] for step in record["steps"]] == labels, "complete ordered terminal process membership")


def controls(record):
    validate(record)
    changes = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_model", lambda r: r["models"].pop()),
        ("lost_preparation", lambda r: r["preparation_steps"].clear()),
        ("boolean_exit", lambda r: r["steps"][-1].update(exit_code=False)),
        ("changed_command", lambda r: r["steps"][-1].update(command=["true"])),
        ("changed_input", lambda r: r["steps"][-1].update(input="{}\n")),
        ("changed_stdout", lambda r: r["steps"][-1].update(stdout="{}\n")),
        ("lost_sdk", lambda r: r["models"][0]["cpp_sources"].pop(next(iter(r["models"][0]["cpp_sources"])))),
        ("wrong_carrier", lambda r: r["models"][0].update(carrier="unqualified")),
        ("changed_resolution", lambda r: r["models"][0].update(resolution_document={})),
        ("lost_scenario", lambda r: r["models"][0]["inputs"].pop()),
        ("changed_observer", lambda r: r.update(cpp_source="int main() {}\n")),
        ("missing_observation", lambda r: r["models"][0]["cpp"].pop()),
        ("wrong_schema", lambda r: r["models"][0]["cpp"][0]["schema"]["reading"][1].update(type="TEXT")),
        ("missing_graph_member", lambda r: r["models"][0]["cpp"][2]["plans"][0]["operations"].pop()),
        ("lost_audit", lambda r: r["models"][0]["cpp"][2]["events"].clear()),
        ("false_rollback", lambda r: r["models"][0]["cpp"][6]["database"]["gateways"][0].update(version=2)),
        ("false_committed_success", lambda r: r["models"][0]["cpp"][9]["outcome"].update(ok=True)),
        ("unreclaimed", lambda r: r.update(owned_directory_removed=False)),
        ("missing_reference", lambda r: r["references_after"].clear()),
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
            raise RuntimeError("Accepted governed scalar corruption: " + name)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--case", action="append")
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.record, args.case)
    else:
        record = read(args.record)
        validate(record)
        if args.controls:
            require(not args.controls.exists(), "preserve preceding corruption evidence")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls(record)))
        print("Verified generated scalar schema and governed execution; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
