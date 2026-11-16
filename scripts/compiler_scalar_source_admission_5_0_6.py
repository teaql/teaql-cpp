#!/usr/bin/env python3
"""Admit compiler scalar storage through exact sources and executed SDK contracts."""

import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "runtime": ("compiler_scalar_storage_typeid_5_0_6.json.gz", "2d30297b3bd82b5ec1ba54a4a92029b62e2a490e055a533e2471942cebff6258"),
    "sdk": ("compiler_scalar_storage_sdks_5_0_6.json.gz", "345007068e127260f0977eda88d506adb4d027ace0cce1c506562b73d613c460"),
    "unit": ("compiler_scalar_storage_units_candidate_5_0_6.json.gz", "ea7f7060a1f06477ebbd8b955ad68b5cd701c045ea47503575f116469a063925"),
    "worker_unit": ("compiler_scalar_storage_typeid_worker_units_5_0_6.json.gz", "15c1bcf2156a26dd290b83eed2d00e0c5333699381744c3a82bf3622c368304b"),
    "source": ("compiler_scalar_storage_source_transition_5_0_6.json.gz", "e57725b5e9823389a0fcb2d4ec58a78e2eeece09ec072a381380e6a27dd9edf8"),
    "readers": ("compiler_scalar_storage_reader_transition_5_0_6.json.gz", "ea7c2df8fd0930fb8298526b2a0e307fcfb6188b6bbe2df3e97ea09b2d5aacb9"),
    "reader_final": ("compiler_scalar_storage_reader_final_transition_5_0_6.json.gz", "39f24b11825e1e5a6b3e051a9e8dacbbe47a983ba63118325c66fd8bb8dbb011"),
    "runtime_controls": ("compiler_scalar_storage_controls_5_0_6.json.gz", "e1082d66d68a7b5a733ee65d5378030ee3cf0c39e02e2def4b1753c3531b434b"),
    "runtime_controls_optimized": ("compiler_scalar_storage_controls_optimized_5_0_6.json.gz", "6efcf1e9a940d6510b83e3865d9570aa68968311c8536d8b105f245c64d1f5a0"),
}
READER_CHANGES = {
    "generator_evaluation_source_admission_5_0_6.py": ["registered_epoch_matches", "current_input_matches", "java_inventory_matches"],
    "verify_compiler_generated_business_5_0_6.py": ["validate"],
    "verify_compiler_governed_execution_5_0_6.py": ["validate"],
    "verify_compiler_model_configuration_5_0_6.py": ["validate"],
    "verify_compiler_scalar_storage_5_0_6.py": ["validate"],
    "verify_cpp_compiler_job_5_0_6.py": ["validate"],
    "verify_generator_evaluation_5_0_6.py": ["validate"],
    "nominal_type_source_admission_5_0_6.py": ["cross_context_input_matches"],
    "reproduce_evaluated_generator_sdks_5_0_6.py": ["load_recipe", "validate"],
}


def require(value, message):
    if not value:
        raise RuntimeError("Compiler scalar source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable executed receipt: " + name)
    return json.loads(gzip.decompress(data))


def source_path(name):
    repository, relative = name.split(":", 1)
    path = Path(relative)
    require(repository in ("runtime", "producer") and not path.is_absolute()
            and ".." not in path.parts, "exact repository-relative source identity")
    return (ROOT if repository == "runtime" else PRODUCER) / path


@cache
def source_epochs():
    from generator_evaluation_source_admission_5_0_6 import source_epochs as preceding_epochs

    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for key in ("source", "readers", "reader_final"):
        for name, row in bound(key)["paths"].items():
            result.setdefault(source_path(name), []).append(row)
    for before, after in zip(bound("sdk")["before"], bound("sdk")["after"], strict=True):
        name = Path(before["manifest"]).name
        result.setdefault(ROOT / before["manifest"], []).append(dict(
            before_sha256=before["files"][name], after_sha256=after["files"][name]))
    return result


def registered_epoch_matches(path, expected):
    """Pure exact-image traversal; evidence validators never call themselves."""
    path = Path(path)
    if not path.is_file():
        return False
    observed = sha(path.read_bytes())
    pending, visited = [expected], set()
    while pending:
        candidate = pending.pop()
        if candidate == observed:
            return True
        if candidate in visited:
            continue
        visited.add(candidate)
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, [])
                       if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    path = Path(path)
    if not path.is_file():
        return False
    if sha(path.read_bytes()) == expected:
        return True
    if not registered_epoch_matches(path, expected):
        return False
    evidence()
    return True


def frame_matches(current, historical):
    return all(name in current and (current[name] == value
               or registered_epoch_matches(source_path(name), value))
               for name, value in historical.items())


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER / "generator/src/main/java")
                 and row.get("before_source", "existing") is None}
    if not unknown <= additions.keys():
        return False
    if unknown:
        evidence()
    return all(Path(name).is_file() and registered_epoch_matches(Path(name), additions[name]) for name in unknown)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_sources(records):
    source, readers = records["source"], records["readers"]
    require(source["complete"] is True and source["whole_goal_complete"] is False
            and len(source["paths"]) == 16, "complete executed implementation/recipe inventory")
    runtime, sdk = records["runtime"], records["sdk"]
    for name, row in source["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact current source image: " + name)
        require(row["before_source"] is None and row["before_sha256"] is None
                or row["before_source"] is not None and row["before_sha256"] == sha(row["before_source"].encode()),
                "exact prior source image")
        frame = sdk["input_hashes_after"] if name.startswith("runtime:scripts/") and (
            name.endswith("generate_examples.py") or name.endswith("reproduce_cpp_contract_fixtures.py")
            or name.endswith("reproduce_cpp_independent_sdks_5_0_6.py")
            or name.endswith("reproduce_compiler_scalar_sdks_5_0_6.py")) else runtime["inputs_after"]
        require(frame.get(name) == row["after_sha256"], "actual successful compiler/SDK execution source: " + name)
    require(readers["complete"] is True and readers["whole_goal_complete"] is False
            and set(readers["paths"]) == {"runtime:scripts/" + name for name in READER_CHANGES},
            "nine bounded source/version reader transitions")
    for name, row in readers["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact historical/current reader images")
        before, after = functions(row["before_source"]), functions(row["after_source"])
        changed = [name for name in before if before[name] != after.get(name)]
        require(changed == row["changed_functions"] == READER_CHANGES[Path(name).name]
                and row["preserved_functions"] == [name for name in before if before[name] == after.get(name)]
                and row["added_functions"] == sorted(set(after) - set(before)), "every reader/domain/control function accounted for")
        if Path(name).name == "generator_evaluation_source_admission_5_0_6.py":
            require(row["added_functions"] == sorted("preceding_" + value for value in changed), "only explicit forwarding functions added")
            tree = {node.name: node for node in ast.parse(row["after_source"]).body if isinstance(node, ast.FunctionDef)}
            for value in changed:
                original = copy.deepcopy(tree["preceding_" + value])
                original.name = value
                require(ast.dump(original, include_attributes=False) == before[value], "original exact admission body retained")
        else:
            require(not row["added_functions"], "no domain/control functions added or replaced")
        if "controls" in before:
            require(before["controls"] == after.get("controls"), "historical corruptions remain exact")
    final = records["reader_final"]
    name = "runtime:scripts/verify_generator_evaluation_5_0_6.py"
    require(final["complete"] is True and final["whole_goal_complete"] is False
            and set(final["paths"]) == {name}, "one final evaluator driver identity guard")
    row = final["paths"][name]
    before, after = functions(row["before_source"]), functions(row["after_source"])
    require(row["before_source"] == readers["paths"][name]["after_source"]
            and row["before_sha256"] == sha(row["before_source"].encode())
            and row["after_sha256"] == sha(row["after_source"].encode())
            and registered_epoch_matches(source_path(name), row["after_sha256"])
            and before.keys() == after.keys()
            and [name for name in before if before[name] != after[name]] == ["validate"],
            "exact self-identity continuation preserves all evaluator/control bodies")
    worker = source["paths"]["producer:scripts/cpp_rust_compiler_job.py"]
    before, after = functions(worker["before_source"]), functions(worker["after_source"])
    require([name for name in before if before[name] != after.get(name)]
            == ["load_capabilities", "validate_inventory", "generate"]
            and set(after) - set(before) == {"scalar_source", "validate_scalar_types"}, "only explicit scalar capability branches change the worker")


def validate(records):
    from verify_compiler_scalar_storage_5_0_6 import validate as validate_runtime
    import reproduce_compiler_scalar_sdks_5_0_6 as sdk

    validate_sources(records)
    validate_runtime(records["runtime"])
    engine = sdk.preceding
    names = ("__file__", "BASE_COMMIT", "BASE", "BASE_SHA", "load_recipe", "expected_manifest")
    saved = {name: getattr(engine, name) for name in names}
    try:
        sdk.validate(records["sdk"])
    finally:
        for name, value in saved.items():
            setattr(engine, name, value)
    unit = records["unit"]
    require(unit["complete"] is True and unit["whole_goal_complete"] is False
            and unit["inputs_before"] == unit["inputs_after"]
            and unit["owned_directory_removed"] is True and not Path(unit["owned_directory"]).exists(), "actual complete fresh Java contracts and cleanup")
    require([step["label"] for step in unit["steps"]] == ["current_cpp_javac", "current_cpp_junit", "worker_0", "worker_1"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in unit["steps"])
            and re.search(r"OK \(262 tests\)", unit["steps"][1]["stdout"]), "all 262 actual C++ generator Java tests")
    java = {name for name in unit["steps"][0]["command"] if name.endswith(".java")}
    require(java and all("producer:" + str(Path(name).relative_to(PRODUCER)) in unit["inputs_after"]
            and registered_epoch_matches(Path(name), unit["inputs_after"]["producer:" + str(Path(name).relative_to(PRODUCER))])
            for name in java), "every freshly compiled Java input remains registered")
    worker = records["worker_unit"]
    require(worker["complete"] is True and worker["whole_goal_complete"] is False
            and [step["optimized"] for step in worker["steps"]] == [0, 1]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    and re.search(r"Ran 15 tests\b", step["stderr"]) and step["stderr"].rstrip().endswith("OK")
                    for step in worker["steps"]), "final TypeId worker passes all fifteen ordinary/optimized contracts")
    for mode in (0, 1):
        controls = records["runtime_controls" + ("_optimized" if mode else "")]
        require(controls["complete"] is True and type(controls["optimized"]) is int and controls["optimized"] == mode
                and controls["execution_sha256"] == RECORDS["runtime"][1] and len(controls["controls"]) == 14
                and all(row["rejected"] is True for row in controls["controls"]), "all actual ordinary/optimized scalar corruptions reject")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


def controls():
    records = evidence()
    mutations = (
        ("runtime", "incomplete_runtime", lambda r: r.update(complete=False)),
        ("runtime", "missing_scalar_model", lambda r: r["cases"].pop()),
        ("runtime", "wrong_scalar_carrier", lambda r: r["cases"][0].update(carrier="double")),
        ("runtime", "changed_original_output", lambda r: r["steps"][-2].update(stdout="{}\n")),
        ("sdk", "missing_sdk", lambda r: r["after"].pop()),
        ("sdk", "changed_generated_body", lambda r: r["after"][0]["files"].update(invented="0" * 64)),
        ("unit", "wrong_java_count", lambda r: r["steps"][1].update(stdout="OK (259 tests)\n")),
        ("worker_unit", "missing_optimized_worker", lambda r: r["steps"].pop()),
        ("source", "unqualified_source", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("readers", "changed_control_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("reader_final", "changed_driver_guard", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("runtime_controls_optimized", "lost_optimized_corruption", lambda r: r["controls"].pop()),
    )
    result = []
    for key, name, mutate in mutations:
        changed = dict(records)
        changed[key] = copy.deepcopy(records[key])
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted scalar source corruption: " + name)
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set()), "unknown Java source rejected")
    result.append(dict(case="unknown_java_source", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve earlier source admission controls")
        result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls(),
                      receipts={key: value[1] for key, value in RECORDS.items()})
        args.controls.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
        print(json.dumps(dict(complete=True, optimized=sys.flags.optimize, controls=len(result["controls"]))))
    else:
        print("Verified exact compiler scalar storage sources, SDKs, execution and contracts")


if __name__ == "__main__":
    main()
