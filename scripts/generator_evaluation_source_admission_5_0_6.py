#!/usr/bin/env python3
"""Admit shared evaluator repair through exact source images and executed gates."""

import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "unit": ("generator_evaluation_full_5_0_6.json.gz", "b3623d2621aff942cc21a69b5ef90b07e57edc48709dea057f9d2a1d4045ed04"),
    "sdk": ("generator_evaluation_sdks_current_5_0_6.json.gz", "b68be40d0767ee1b77f18bf1eb7513e08eaa22d75e0a82c1b78396b42a239749"),
    "default": ("generator_evaluation_default_sdks_5_0_6.json.gz", "436bfd2d02bc958b1faf78d8cf8592f0d201c3e4c060de872f3034a0795c4bd2"),
    "controller_complete": ("generator_evaluation_controller_complete_5_0_6.json.gz", "dd55aac44211e5e1f850d52797ee9c6e4570ebb09a5cdbf81e26cdb76e88bf60"),
    "controller_partial": ("generator_evaluation_controller_partial_5_0_6.json.gz", "c38d5a95d4997d40d5ac07c2bb8de970134ecf8c7a7310579d1f5c3a364ab134"),
    "source": ("generator_evaluation_source_transition_5_0_6.json.gz", "bb23df67e39f64c6b8ef367fd366567e8c28cd5cfbab7ba7c05276ed3d406767"),
    "readers": ("generator_evaluation_reader_transition_5_0_6.json.gz", "718ff071c7e92f7f0d6767e4666b6d5ffc231aa6cad72d384829f7802da1f1fb"),
    "inventory": ("generator_evaluation_inventory_current_transition_5_0_6.json.gz", "20790bfa3b42344c1a9bb0b240c0910e718bcceb3a9c0012828547ab7b08730b"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Generator evaluation source admission: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable qualification receipt: " + name)
    return json.loads(gzip.decompress(data))


def source_path(name):
    repository, relative = name.split(":", 1)
    require(repository in ("runtime", "producer"), "known repository")
    return (ROOT if repository == "runtime" else PRODUCER) / relative


@cache
def source_epochs():
    from compiler_model_configuration_source_admission_5_0_6 import source_epochs as preceding_epochs

    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for key in ("source", "readers", "inventory"):
        for name, row in bound(key)["paths"].items():
            result.setdefault(source_path(name), []).append(row)
    for before, after in zip(bound("sdk")["before"], bound("sdk")["after"], strict=True):
        path = before["manifest"]
        name = Path(path).name
        result.setdefault(ROOT / path, []).append(dict(before_sha256=before["files"][name],
                                                        after_sha256=after["files"][name]))
    return result


def registered_epoch_matches(path, expected):
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
    if not path.is_file():
        return False
    if sha(path.read_bytes()) == expected:
        return True
    if not registered_epoch_matches(path, expected):
        return False
    evidence()
    return True


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER / "generator/src/main/java")
                 and row.get("before_source", "existing") is None}
    if not unknown <= additions.keys():
        return False
    if unknown:
        evidence()
    return all(Path(name).is_file() and sha(Path(name).read_bytes()) == additions[name] for name in unknown)


def validate(records):
    from verify_generator_evaluation_5_0_6 import imported_sources, validate as validate_unit
    from reproduce_evaluated_generator_sdks_5_0_6 import validate as validate_sdks
    from verify_compiler_governed_execution_5_0_6 import validate as validate_business
    import preserve_compiler_model_contracts_5_0_6 as default

    validate_unit(records["unit"])
    validate_sdks(records["sdk"])
    prior_source = default.SOURCE
    try:
        default.SOURCE = EVIDENCE / RECORDS["unit"][0]
        default.validate(records["default"])
    finally:
        default.SOURCE = prior_source
    for name in ("complete", "partial"):
        record = records["controller_" + name]
        sinks = EVIDENCE / ("generator_evaluation_controller_" + name + "_sinks_5_0_6.json.gz")
        require(record["controller"] is True and record["entry"] == "controller", "actual controller path")
        validate_business(record, json.loads(gzip.decompress(sinks.read_bytes())))
    original = ROOT / ".reference/teaql_code_gen"
    for relative in imported_sources():
        require((original / relative).read_bytes() == (PRODUCER / relative).read_bytes(),
                "complete locked shared implementation: " + str(relative))
    for kind in ("source", "readers", "inventory"):
        transition = records[kind]
        require(transition["complete"] is True and transition["whole_goal_complete"] is False
                and transition["paths"], "complete exact source transition")
        for name, row in transition["paths"].items():
            require(row["after_sha256"] == sha(row["after_source"].encode())
                    and registered_epoch_matches(source_path(name), row["after_sha256"]),
                    "actual registered source continuation: " + name)
            if row["before_source"] is not None:
                require(row["before_sha256"] == sha(row["before_source"].encode()), "exact source preimage")
    for row in records["readers"]["paths"].values():
        functions = lambda text: {node.name: ast.dump(node, include_attributes=False)
                                  for node in ast.parse(text).body
                                  if isinstance(node, (ast.FunctionDef, ast.ClassDef))}
        before, after = functions(row["before_source"]), functions(row["after_source"])
        require([name for name in before if before[name] != after.get(name)] == row["changed_functions"]
                and [name for name in before if before[name] == after.get(name)] == row["preserved_functions"],
                "exact named reader/control body preservation")
        if "controls" in before:
            require(before["controls"] == after.get("controls"), "historical corruption-control body is exact")
    require(set(records["inventory"]["paths"]) == {"runtime:scripts/nominal_type_source_admission_5_0_6.py",
             "runtime:scripts/compiler_source_service_source_admission_5_0_6.py"},
            "two exact source-graph inventory/version boundaries")
    for row in records["inventory"]["paths"].values():
        restored = row["before_source"]
        require(len(row["edits"]) == 1, "one source-graph import substitution")
        for edit in row["edits"]:
            require(restored.count(edit["before"]) == 1, "exact old graph-discovery import")
            restored = restored.replace(edit["before"], edit["after"], 1)
        require(restored == row["after_source"], "all other inventory data/control bodies preserved")
    require(records["unit"]["original_shared_test_count"] == records["unit"]["cpp_shared_test_count"] == 95
            and records["unit"]["cpp_generator_test_count"] == 259, "actual complete shared/generator tests")
    require(records["default"]["actual_cpp_compiler_processes"] == 0
            and records["default"]["reused_cpp_compiler_outcomes"] == 406, "default SDK compiler outcomes explicitly reused")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


def controls():
    valid = evidence()
    changes = (
        ("unit", "incomplete_units", lambda r: r.update(complete=False)),
        ("unit", "missing_evaluation", lambda r: r["cases"].pop()),
        ("unit", "wrong_shared_count", lambda r: r.update(cpp_shared_test_count=46)),
        ("sdk", "missing_sdk", lambda r: r["after"].pop()),
        ("sdk", "changed_generated_code", lambda r: r["after"][0]["files"].update(unknown="0" * 64)),
        ("default", "missing_default_sdk", lambda r: r["cases"].pop()),
        ("default", "invented_compiler", lambda r: r.update(actual_cpp_compiler_processes=406)),
        ("controller_complete", "missing_controller", lambda r: r.update(controller=False)),
        ("controller_partial", "missing_graph_write", lambda r: r["cpp_save"]["graph_success"].update(enrollment_note="student")),
        ("source", "wrong_source_image", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("readers", "wrong_reader_image", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("inventory", "wrong_inventory_image", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
    )
    result = []
    for key, name, mutate in changes:
        changed = copy.deepcopy(valid)
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted shared-evaluation evidence corruption: " + name)
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set()), "unknown Java source rejected")
    result.append(dict(case="unqualified_java_source", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), "preserve source controls")
    rejected = controls()
    args.output.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
        optimized=sys.flags.optimize, controls=rejected, record_hashes={key: value[1] for key, value in RECORDS.items()}),
        sort_keys=True).encode(), mtime=0))
    print("Verified shared evaluation source admission:", len(rejected), "controls; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
