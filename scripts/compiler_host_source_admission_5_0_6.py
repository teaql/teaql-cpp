#!/usr/bin/env python3
"""Qualify trusted compiler host assembly and preserve historical source gates."""

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
    "runtime": ("compiler_host_assembly_5_0_6.json.gz", "175baac560fe4d354a4a5ffb090856db69df80e6362a23eb3c06072b8dd2bee3"),
    "units": ("compiler_host_units_5_0_6.json.gz", "738d78bfc69b62dae4b99094bcaf223e1d6ee230111c9310da959cfce0a69f25"),
    "documents": ("compiler_host_document_transition_5_0_6.json.gz", "e14b8d53fcc8fbb345887d0277c645bfa2b7ade2987c478a48b5db86b1e3571d"),
    "source": ("compiler_host_source_transition_5_0_6.json.gz", "2720dfb58ee2e45de568c7137924258fc100fa2d2961e4cee40bf4413b664710"),
    "readers": ("compiler_host_reader_transition_5_0_6.json.gz", "165f54fdd3265f0d7ce60713b65c7c34d6fadf8c18b67e8e0c8998cd43fd1cd7"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Compiler host admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable actual receipt: " + name)
    return json.loads(gzip.decompress(data))


def source_path(name):
    repository, relative = name.split(":", 1)
    path = Path(relative)
    require(repository in ("runtime", "producer") and not path.is_absolute() and ".." not in path.parts,
            "repository-relative source identity")
    return (ROOT if repository == "runtime" else PRODUCER) / path


@cache
def source_epochs():
    from narrow_integer_source_admission_5_0_6 import source_epochs as preceding_epochs
    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for key in ("source", "documents", "readers"):
        for name, row in bound(key)["paths"].items():
            result.setdefault(source_path(name), []).append(row)
    return result


def registered_epoch_matches(path, expected):
    from compiler_host_governance_source_admission_5_0_6 import registered_epoch_matches as current
    return current(path, expected)


def current_input_matches(path, expected):
    from compiler_host_governance_source_admission_5_0_6 import current_input_matches as current
    return current(path, expected)


def frame_matches(current, historical):
    from compiler_host_governance_source_admission_5_0_6 import frame_matches as matches
    return matches(current, historical)


def java_inventory_matches(current, historical):
    from compiler_host_governance_source_admission_5_0_6 import java_inventory_matches as matches
    return matches(current, historical)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_sources(records):
    source, documents, readers = records["source"], records["documents"], records["readers"]
    runtime, units = records["runtime"], records["units"]
    require(source["complete"] is True and source["whole_goal_complete"] is False
            and source["producer_base"] == "02027c04750ce7e8c93fd660716badc6c6a8cbe8"
            and len(source["paths"]) == 6, "complete bounded host source inventory")
    for name, row in source["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact qualified source image")
        require(row["before_source"] is None and row["before_sha256"] is None
                or row["before_source"] is not None and row["before_sha256"] == sha(row["before_source"].encode()), "exact source preimage")
        if name.endswith(".java"):
            observed = units["source_images"][str(source_path(name))]
            require(observed["before_source"] == row["before_source"] and observed["after_source"] == row["after_source"]
                    and units["prepared_java_sources"][str(source_path(name))] == row["after_sha256"], "actual freshly compiled Java bytes")
        elif name.endswith(".md"):
            require(row["after_sha256"] == runtime["inputs_before"][name], "actual initial host documentation bytes")
        else:
            require(row["after_source"] == runtime["driver_source"]
                    and row["after_sha256"] == runtime["inputs_after"][name], "actual unchanged execution driver")
    controller = source["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/GenerateController.java"]
    require(controller["after_source"].replace("    @org.springframework.beans.factory.annotation.Autowired\n", "", 1)
            == controller["before_source"], "controller changes only constructor injection annotation")
    require(documents["complete"] is True and documents["whole_goal_complete"] is False
            and type(documents["initial_cli_exit_code"]) is int and documents["initial_cli_exit_code"] == 1
            and documents["execution_steps_all_successful"] is True
            and documents["execution_receipt_sha256"] == RECORDS["runtime"][1]
            and set(documents["paths"]) == {"producer:docs/cpp_compiler_host.md"}, "preserved initial final-frame failure")
    for name, row in documents["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and row["before_sha256"] == runtime["inputs_before"][name]
                and row["after_sha256"] == runtime["inputs_after"][name]
                and registered_epoch_matches(source_path(name), row["after_sha256"])
                and row["before_source"].replace("Any supplied compiler property enables configuration validation.",
                    "Any supplied setting from this table enables configuration validation.", 1) == row["after_source"],
                "exact one-sentence documentation continuation; no executed code changes")
    require(readers["complete"] is True and readers["whole_goal_complete"] is False
            and set(readers["paths"]) == {"runtime:scripts/narrow_integer_source_admission_5_0_6.py",
                                         "runtime:scripts/verify_compiler_host_assembly_5_0_6.py"}, "two bounded source/receipt readers")
    for name, row in readers["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact reader source images")
        before, after = functions(row["before_source"]), functions(row["after_source"])
        changed = [key for key in before if before[key] != after.get(key)]
        if name.endswith("verify_compiler_host_assembly_5_0_6.py"):
            require(changed == ["main"] and set(after) - set(before) == {"validate", "controls"}
                    and row["before_source"] == runtime["driver_source"], "all actual execution/case/command bodies remain unchanged")
        else:
            require(changed == ["registered_epoch_matches", "current_input_matches", "frame_matches", "java_inventory_matches"]
                    and before.keys() == after.keys(), "only matching delegates change; retained checks stay exact")


def validate_units(record):
    from verify_compiler_generated_business_5_0_6 import execution_inputs, references
    actual = bound("units")
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["inputs_after"].keys() == actual["inputs_after"].keys()
            and record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and record["references_before"] == record["references_after"] == references()
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "complete stable owned Java execution")
    steps = record["steps"]
    require([step["label"] for step in steps] == ["current_javac", "current_host_contracts", "current_all_contracts",
            "prior_controller_javac", "prior_host_contracts"], "actual candidate and preceding-controller command membership")
    for step, executed in zip(steps, actual["steps"], strict=True):
        require(step["command"] == executed["command"] and type(step["exit_code"]) is int
                and step["exit_code"] == (1 if step["label"] == "prior_host_contracts" else 0), "exact actual Java commands and exit codes")
    require(steps[0]["command"][:4] == ["javac", "--release", "17", "-cp"]
            and set(record["prepared_java_sources"]) == {name for name in steps[0]["command"] if name.endswith(".java")}
            and all(registered_epoch_matches(Path(name), digest) for name, digest in record["prepared_java_sources"].items()), "complete freshly compiled current Java sources")
    require(re.search(r"OK \(7 tests\)", steps[1]["stdout"]) is not None
            and re.search(r"OK \(272 tests\)", steps[2]["stdout"]) is not None
            and "Tests run: 7,  Failures: 1" in steps[4]["stdout"]
            and "springControllerUsesConfiguredCompilerForUploadedModels" in steps[4]["stdout"], "new host regressions pass and catch exact preceding controller failure")


def validate(records):
    from verify_compiler_host_assembly_5_0_6 import validate as validate_runtime
    validate_sources(records)
    validate_units(records["units"])
    validate_runtime(records["runtime"])


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


def controls():
    records = evidence()
    mutations = (
        ("units", "incomplete_units", lambda r: r.update(complete=False)),
        ("units", "boolean_unit_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("units", "empty_unit_frames", lambda r: r.update(inputs_before={}, inputs_after={})),
        ("units", "substituted_junit_command", lambda r: r["steps"][2].update(command=["true"])),
        ("units", "missing_prior_failure", lambda r: r["steps"][4].update(exit_code=0)),
        ("units", "lost_java_test", lambda r: r["steps"][1].update(stdout="OK (0 tests)")),
        ("source", "forged_source", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("readers", "changed_execution_body", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("documents", "hidden_initial_failure", lambda r: r.update(initial_cli_exit_code=0)),
        ("documents", "boolean_initial_failure", lambda r: r.update(initial_cli_exit_code=True)),
        ("documents", "unqualified_document", lambda r: next(iter(r["paths"].values())).update(after_source="forged")),
        ("runtime", "missing_business_consumer", lambda r: r["consumers"].pop()),
        ("runtime", "unremoved_host_job", lambda r: r.update(owned_directory_removed=False)),
    )
    result = []
    for key, name, mutate in mutations:
        changed = dict(records)
        changed[key] = copy.deepcopy(records[key])
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler host source corruption: " + name)
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set())
            and not registered_epoch_matches(ROOT / "tests/CMakeLists.txt", "0" * 64), "unknown Java sources and source hashes reject")
    result.append(dict(case="unknown_source", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve previous host controls")
        result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls())
        args.controls.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
        print("Verified host source and", len(result["controls"]), "controls; optimized=", sys.flags.optimize)
    else:
        print("Verified compiler host source, actual business execution and preserved initial admission failure")


if __name__ == "__main__":
    main()
