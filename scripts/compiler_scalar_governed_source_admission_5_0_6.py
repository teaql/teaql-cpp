#!/usr/bin/env python3
"""Admit the exact scalar persistence observers and bounded reader continuation."""

import argparse
import ast
import copy
from functools import cache
from pathlib import Path
import sys

from verify_compiler_generated_business_5_0_6 import read, require, sha, write

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "date": ("compiler_scalar_governed_date_floor_5_0_6.json.gz", "aee3ec7897425dac617f4f4bd9b69a38e68bc10c0eac9a86d5bb66d89a5255a9"),
    "remaining": ("compiler_scalar_governed_optional_arguments_5_0_6.json.gz", "9c15649a2a057f38c7e81734f5ef490a89f55fe752560a4256af995725eeb19f"),
    "temporal_source": ("compiler_host_temporal_json_source_continuation_5_0_6.json.gz", "512d83e4f890b1754dab2148381155eb3e5061de1120d6c81b90999e12e81706"),
    "source": ("compiler_scalar_governed_sources_final_5_0_6.json.gz", "e95213541b503077e4aad9fa2a700cd098602e93f80d6c5f96efa14f230773c2"),
}
COLLECTOR = "runtime:scripts/verify_compiler_scalar_governed_5_0_6.py"
TEMPORAL = "runtime:scripts/verify_compiler_host_temporal_json_5_0_6.py"
CPP = "runtime:tests/parity/compiler_scalar_governed_probe.cpp"
RUST = "runtime:tests/parity/compiler_scalar_governed_probe.rs"
LEAF = "runtime:scripts/compiler_scalar_governed_source_admission_5_0_6.py"


@cache
def bound(key):
    name, expected = RECORDS[key]
    path = EVIDENCE / name
    require(sha(path.read_bytes()) == expected, "immutable scalar continuation receipt: " + name)
    return read(path)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_sources(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {COLLECTOR, TEMPORAL, CPP, RUST}, "four exact source images; no production source changes")
    for name, row in record["paths"].items():
        require(name.startswith("runtime:") and ".." not in Path(name.split(":", 1)[1]).parts, "runtime-relative source")
        path = ROOT / name.split(":", 1)[1]
        require(row["after_sha256"] == sha(row["after_source"].encode()) == sha(path.read_bytes()), "exact current source image")
        require(row["before_source"] is None and row["before_sha256"] is None
                or row["before_source"] is not None and row["before_sha256"] == sha(row["before_source"].encode()), "exact source preimage")
    rows = record["paths"]
    require(all(rows[name]["before_source"] is None for name in (COLLECTOR, CPP, RUST)), "three new fixture/collector sources")
    original, after = functions(bound("remaining")["driver_source"]), functions(rows[COLLECTOR]["after_source"])
    require(set(after) - set(original) == {"check_native_outcome", "validate", "controls"}
            and [key for key in original if original[key] != after.get(key)] == ["main"],
            "unchanged executed generation, input and observer adapter bodies; reader additions only")
    original, after = functions(rows[TEMPORAL]["before_source"]), functions(rows[TEMPORAL]["after_source"])
    require(original.keys() == after.keys() and [key for key in original if original[key] != after[key]] == ["validate"],
            "previous temporal execution/commands/datasets and fifteen controls remain exact")
    previous = bound("temporal_source")["paths"][TEMPORAL]
    require(set(previous) == {"initial", "untyped"} and all(
        rows[TEMPORAL]["before_source"] == transition["source_after"]
        and transition["source_before_sha256"] == sha(transition["source_before"].encode())
        and transition["source_after_sha256"] == sha(transition["source_after"].encode())
        for transition in previous.values()), "contiguous preceding temporal reader checkpoint")
    for key in ("date", "remaining"):
        actual = bound(key)
        require(actual["complete"] is True and actual["inputs_before"] == actual["inputs_after"]
                and actual["cpp_source"] == rows[CPP]["after_source"] and actual["rust_source"] == rows[RUST]["after_source"]
                and actual["inputs_after"][COLLECTOR] == sha(actual["driver_source"].encode()), "unchanged actual successful observers and frozen execution collectors")


@cache
def evidence():
    record = bound("source")
    validate_sources(record)
    return record


def source_matches(path, expected):
    from compiler_scalar_consumer_source_admission_5_0_6 import source_matches as matches
    return matches(path, expected)


def frame_matches(current, historical):
    from compiler_scalar_consumer_source_admission_5_0_6 import frame_matches as matches
    return matches(current, historical)


def controls():
    record = evidence()
    mutations = (
        ("unfinished", lambda r: r.update(complete=False)),
        ("missing_source", lambda r: r["paths"].pop(CPP)),
        ("forged_image", lambda r: r["paths"][RUST].update(after_source="fn main() {}\n")),
        ("changed_preimage", lambda r: r["paths"][TEMPORAL].update(before_sha256="0" * 64)),
    )
    result = []
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate_sources(changed)
        except (RuntimeError, KeyError, ValueError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted scalar source corruption: " + name)
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    current = execution_inputs()
    require(not frame_matches(dict(current, **{"runtime:unknown.cpp": "0" * 64}), bound("remaining")["inputs_after"])
            and not frame_matches({name: value for name, value in current.items() if name != CPP}, bound("remaining")["inputs_after"])
            and not source_matches(ROOT / "tests/CMakeLists.txt", "0" * 64), "unknown additions, missing sources and unqualified hashes reject")
    result.append(dict(case="unknown_or_missing_source", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve source corruption evidence")
        write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls()))
    print("Verified exact scalar observer/collector/reader source continuation; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
