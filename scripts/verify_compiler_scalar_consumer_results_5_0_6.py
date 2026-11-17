#!/usr/bin/env python3
"""Check complete scalar consumers and independent field/state expectations."""

import argparse
import copy
from datetime import datetime, timedelta, timezone
from pathlib import Path
import sys

from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, read, references, require, sha, write
import verify_compiler_scalar_consumers_5_0_6 as consumers
import verify_compiler_scalar_family_5_0_6 as family
from verify_compiler_model_configuration_5_0_6 import worker_module

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same


def diagnostic(rule, field, message, profile):
    root = ("readingInfo", "reading_info", "ReadingInfo")[profile]
    name = field
    if profile == 0 and field == "value_issue":
        name = "valueIssue"
    if profile == 2:
        name = field.title().replace("_", "")
    return dict(ruleId=rule, instancePath="/2/" + root + "/" + name,
        location=[dict(kind="index", index=2), dict(kind="property", name="reading_info"), dict(kind="property", name=field)],
        message=message, inputValue=None, systemValue=None, entityType=None, sourceInstancePath=None)


def check_checker(data, output):
    initial = [True, data["loaded"] == "all", data["loaded"] != "identity", True]
    gateway = data["gateway"]
    fixed = data["status"] == "create" and gateway == 0 and data["root"] is not None
    if fixed:
        gateway = data["root"]
        initial[2] = True
    errors = []
    if data["status"] == "create" or data["status"] == "update" and gateway != 0:
        if data["root"] is None:
            errors.append(("context_root_missing", "gateway", "active root gateway is missing from UserContext"))
        elif gateway != data["root"]:
            errors.append(("context_root_mismatch", "gateway", None))
    if data["status"] == "update":
        for index, field in ((2, "gateway"), (1, "value")):
            if not initial[index]:
                errors.append(("invalid_type", field, "Mutation requires a fully loaded entity"))
    call = dict(before=data["value"], gateway=gateway, errors=len(errors), location="[2].reading_info")
    if data["logic"] == "reject":
        errors.append(("required", "value_issue", None))
    expected_value = data["next_value"] if data["logic"] == "fix" else data["value"]
    if data["logic"] == "fix":
        initial[1] = True
    # Direct TypedChecker calls have no graph evidence-publication scope.
    fixes = []
    expected = dict(value=expected_value, gateway=gateway, loaded=initial, calls=[call], fixes=fixes,
                    diagnostics=[[diagnostic(*error, profile) for error in errors] for profile in range(3)])
    require(same(output, expected), "independent typed Checker value/load/Fix/callback/diagnostic expectation")


def sample_value(case, index, seed):
    kind = case["kind"]
    days = (index + seed) % 1095
    if kind == "date":
        return (datetime(1970, 1, 1, tzinfo=timezone.utc) + timedelta(milliseconds=consumers.WALL, days=-days)).date().isoformat()
    if kind == "timestamp":
        return consumers.WALL - days * 86400000
    if kind == "text":
        return "string() " + str(index + 1)
    if kind == "bool":
        return 0
    value = (index + seed) % 1000 + 1
    return float(value) if kind in ("f32", "f64") else value


def check_sample(data, output, case):
    require(type(output) is list and len(output) == data["rounds"], "complete actual sample rounds")
    accumulated = []
    for round in output:
        if not data["root"]:
            expected = dict(report=dict(error=None, generated={}, skipped=[dict(entity="reading", reason="Required dependency gateway is missing in reference pool")]), rows=[])
        elif case["kind"] == "json":
            expression = "jsonMe()" if case["optional"] else "jsonMe(optional=false)"
            error = 'check failed: failed to materialize Reading for checker: Reading: invalid field value: Text("' + expression + ' 1")'
            expected = dict(report=dict(error=error), rows=[])
        elif data["fault"] == "trigger":
            expected = dict(report=dict(error="graph write error: Transport error: fixture sample insert failed"), rows=[])
        else:
            count = dict(Tiny=20, Small=100, Medium=1000)[data["scale"]]
            first = len(accumulated)
            accumulated.extend(dict(id=first + index + 1, gateway=1, version=1, value=sample_value(case, index, data["seed"])) for index in range(count))
            expected = dict(report=dict(error=None, generated=dict(reading=count), skipped=[]), rows=accumulated)
        require(same(round, expected), "independent sample report, count, IDs, values, repeat, dependency and trigger expectation")


def check_consumer(data, output, case):
    require(set(output) == {"wall_millis", "result"} and type(output["wall_millis"]) is int
            and output["wall_millis"] == consumers.WALL == data["wall_millis"], "actual controlled process wall clock")
    if data["kind"] == "checker":
        check_checker(data, output["result"])
    else:
        check_sample(data, output["result"], case)


def stored_value(case, value):
    if value is None:
        return None
    if case["kind"] == "bool":
        return int(value)
    if case["kind"] in ("f32", "f64", "decimal"):
        return float(value)
    return value


def check_graph(data, output, case):
    mode = data["mode"]
    committed = mode in ("create", "update", "graph", "approved", "sink_fail")
    graph = mode not in ("create", "update")
    require(same(output["outcome"], dict(ok=mode in ("create", "update", "graph", "approved"),
        policy_error=mode == "deny", conflict=mode.startswith("stale_"), committed_error=mode == "sink_fail")), "independent governed outcome classification")
    value = data["next_value"] if mode != "create" and committed else data["value"]
    require(same(output["database"], dict(
        gateways=[dict(id=1, name="UPDATED-ROOT" if graph and committed else "string()", version=2 if graph and committed or mode == "stale_root" else 1)],
        readings=[dict(id=7, gateway=1, value=stored_value(case,value), version=2 if mode != "create" and committed or mode == "stale_child" else 1)],
    )), "independent actual stored graph values and rollback/conflict/commit versions")
    require(len(output["plans"]) == 1 and len(output["plans"][0]["operations"]) == (2 if graph else 1)
            and len(output["events"]) == ((2 if graph else 1) if committed else 0), "whole-plan and post-commit audit membership")


def snapshot():
    return dict(inputs=execution_inputs(), native=native_inputs(), references=references(), clock=sha(consumers.CLOCK.read_bytes()))


def validate(record, key, state=None):
    from compiler_scalar_consumer_source_admission_5_0_6 import bound, frame_matches, source_matches
    from compiler_dynamic_route_source_admission_5_0_6 import registered_epoch_matches
    current = snapshot() if state is None else state
    canonical = bound(key)
    is_family = key in ("text", "family")
    driver = family if is_family else consumers
    require(record["complete"] is True and record["whole_goal_complete"] is False, "complete bounded scalar execution")
    require(record["inputs_before"] == record["inputs_after"] and frame_matches(current["inputs"],record["inputs_after"])
            and source_matches(Path(driver.__file__),sha(record["driver_source"].encode())), "actual frozen and exactly admitted source execution")
    require(record["references_before"] == record["references_after"] == current["references"]
            and record["native_before"] == record["native_after"] == current["native"]
            and record["clock_before"] == record["clock_after"] == current["clock"], "exact reference, native and clock artifacts")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0, "owned builds and consumers reclaimed")
    for name in ("capabilities", "environment_overrides", "process_clock", "preparation_seed_sha256", "prepared_java_sources", "preparation_steps", "steps"):
        require(same(record[name],canonical[name]), "immutable actual compiler/consumer process bytes: "+name)
    require(all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["steps"] + record["preparation_steps"]), "all actual terminal commands succeeded")
    require(all(registered_epoch_matches(Path(name),digest) for name,digest in record["prepared_java_sources"].items()), "exact qualified freshly compiled Java source inputs")
    if is_family:
        require(record["selected_cases"] == canonical["selected_cases"]
                and registered_epoch_matches(family.HOST,sha(record["host_source"].encode()))
                and record["worker_source"] == family.WORKER.read_text(), "actual selected families and current compiler host/worker")
        expected = {case["name"]:case for case in family.cases()}
        rows = record["cases"]
        require([row["name"] for row in rows] == record["selected_cases"], "all exact selected scalar families")
        require(record["observer_sources"] == {name:dict(cpp=cpp.read_text(),rust=rust.read_text()) for name,(cpp,rust) in family.SOURCES.items()}, "all three unchanged business observer templates")
    else:
        require(record["selected_models"] == canonical["selected_models"] and record["compiler_jobs_reused"] is True,
                "explicit original compiler-job reuse and exact selected scalar models")
        expected = {row["case"]["name"]:row["case"] for row in consumers.selected_models(None)}
        rows = record["models"]
        require([row["name"] for row in rows] == record["selected_models"] and record["cpp_source"] == consumers.CPP.read_text()
                and record["rust_source"] == consumers.RUST.read_text(), "complete unchanged direct Checker/sample observers")
    steps = {step["label"]:step for step in record["steps"]}
    require(len(steps) == len(record["steps"]), "unique actual process labels")
    worker = worker_module()
    for row in rows:
        case = expected[row["name"]]
        require(all(same(row[name],case[name]) for name in ("name","kind","optional","carrier")), "exact scalar identity and optionality")
        job = row["job"] if is_family else case["job"]
        if is_family:
            require(row["source_package"] == family.package(case["model"]) and row["model"] == case["model"]
                    and row["job"] == next(value["job"] for value in canonical["cases"] if value["name"] == row["name"]), "actual complete compiler job and input model")
            worker.validate_configuration(job["original_inventory"]["entities"],job["descriptors"],job["configuration"])
            worker.validate_semantic_storage(job["original_inventory"]["entities"],job["configuration"],job["scalar_types"])
            values = [p for e in job["scalar_types"] for p in e["properties"] if p["name"] == "value"]
            require(len(values) == 1 and values[0]["rust_type"] == values[0]["storage_type"] == case["rust_type"]
                    and values[0]["semantic_kind"] == "scalar" and values[0]["semantic_type"] is None, "same-library actual compiler storage identity")
        else:
            source = next(value for value in consumers.selected_models([row["name"]]))
            require(row["compiler_receipt"] == source["compiler_receipt"] and row["compiler_receipt_sha256"] == source["compiler_receipt_sha256"]
                    and row["original_compiler_job_reused"] is True and row["resolution_document"] == case["job"]["resolution_document"], "exact explicitly reused original compiler witness")
        require(job["complete"] is True and job["outcome"] == "resolved" and job["owned_directory_removed"] is True
                and job["publication_directory_removed"] is True and not Path(job["owned_directory"]).exists()
                and not Path(job["publication_directory"]).exists(), "actual complete original/native generation and job cleanup")
        require(row["original_sources"] == row["original_sources_after"] == job["original_sources_before"] == job["original_sources_after"]
                and row["cpp_sources"] == row["cpp_sources_after"] == job["cpp_sources"], "entire independent original and generated C++ libraries unchanged")
        captured = next(value for value in canonical["cases" if is_family else "models"] if value["name"] == row["name"])
        require(row["consumer_manifest"] == captured["consumer_manifest"] and row["consumer_lock"] == captured["consumer_lock"], "actual observer/library manifest binding and reference lock bytes")
        member_names = ("checker","reads","graph") if is_family else ("checker",)
        require(not is_family or set(row["consumers"]) == set(member_names), "all required independently compiled consumer programs")
        for name in member_names:
            member = row["consumers"][name] if is_family else row
            data = family.inputs(name,case) if is_family else consumers.inputs(case)
            require(same(member["inputs"],data) and len(member["cpp"]) == len(member["original"]) == len(data), "every exact field consumer scenario and raw observation")
            prefix = row["name"] + ("_"+name if is_family else "")
            original = steps[prefix+"_original_run"]
            native = steps[prefix+"_native_run"]
            require(same(member["original"],json_lines(original["stdout"])) and same(member["cpp"],json_lines(native["stdout"]))
                    and same(member["original"],member["cpp"]), "complete actual original/native outcomes without normalization")
            if is_family:
                source, adapter = family.rust_observer(name,case)
                require(member["rust_source"] == source and member["rust_argument_adapter"] == adapter, "only exact optional observer argument adaptations")
            else:
                source = consumers.RUST.read_text()
                adapter = None
                if case["optional"] and case["kind"] != "timestamp":
                    before="entity.update_value(desired);"
                    after="entity.update_value(desired.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null));"
                    require(source.count(before) == 2,"two optional observer arguments")
                    source=source.replace(before,after)
                    adapter=dict(before=before,after=after,replacements=2)
                require(member["rust_observer_source"] == source and member["rust_argument_adapter"] == adapter, "unchanged direct original observer with exact argument adaptation")
            for input,output in zip(data,member["cpp"],strict=True):
                if name == "checker":
                    check_consumer(input,output,case)
                elif name == "graph":
                    check_graph(input,output,case)
                else:
                    values = [*case["values"],None] if case["optional"] else case["values"]
                    expected_values = [] if input == data[1] else values
                    require(same([row["getter"] for row in output["rows"]],expected_values)
                            and all(same(row["getter"],row["expression"]) and same(row["getter"],row["json"]["value"]) for row in output["rows"])
                            and output["sql"], "actual typed Query/getter/E/JSON values, empty selection and SQL")


def controls(record, key):
    state = snapshot()
    validate(record,key,state)
    container="cases" if key in ("text","family") else "models"
    changes=(
        ("unfinished",lambda r:r.update(complete=False)),
        ("lost_model",lambda r:r[container].pop()),
        ("lost_preparation",lambda r:r["preparation_steps"].clear()),
        ("boolean_exit",lambda r:r["steps"][-1].update(exit_code=False)),
        ("substituted_command",lambda r:r["steps"][-1].update(command=["true"])),
        ("substituted_input",lambda r:r["steps"][-1].update(input="{}\n")),
        ("substituted_stdout",lambda r:r["steps"][-1].update(stdout="{}\n")),
        ("lost_sdk",lambda r:r[container][0]["cpp_sources"].pop(next(iter(r[container][0]["cpp_sources"])))),
        ("wrong_carrier",lambda r:r[container][0].update(carrier="unqualified")),
        ("wrong_optional",lambda r:r[container][0].update(optional=not r[container][0]["optional"])),
        ("lost_clock",lambda r:r.update(clock_after="0"*64)),
        ("changed_clock_environment",lambda r:r["process_clock"].update(environment={})),
        ("unreclaimed",lambda r:r.update(owned_directory_removed=False)),
        ("lost_reference",lambda r:r["references_after"].clear()),
    )
    results=[]
    for name,mutate in changes:
        changed=copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed,key,state)
        except (RuntimeError,KeyError,IndexError,ValueError) as error:
            results.append(dict(case=name,rejected=True,error=str(error)))
        else:
            raise RuntimeError("Accepted scalar consumer corruption: "+name)
    require(snapshot()==state,"unchanged source/artifacts throughout receipt controls")
    return results


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check",action="store_true",required=True)
    parser.add_argument("--key",choices=("date","remaining","text","family"),required=True)
    parser.add_argument("--record",type=Path,required=True)
    parser.add_argument("--controls",type=Path)
    args=parser.parse_args()
    record=read(args.record)
    validate(record,args.key)
    if args.controls:
        require(not args.controls.exists(),"preserve preceding corruption evidence")
        write(args.controls,dict(complete=True,whole_goal_complete=False,optimized=sys.flags.optimize,controls=controls(record,args.key)))
    print("Verified complete scalar field consumers; optimized=",sys.flags.optimize)


if __name__=="__main__":
    main()
