#!/usr/bin/env python3
"""Resolve current G17 requirement status without rewriting historical matrices."""

import argparse
from collections import Counter
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from collect_source_control_execution_5_0_6 import native_hashes
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references


ROOT = Path(__file__).resolve().parents[1]
UPDATES = ROOT / "docs/governance_qualification_updates_5_0_6.json"
UPDATES_SHA256 = "d7da6c940f9a52d59bf41b87286a31e322f637a0391606cb8e2db9b3b6150918"
BASE_SHA256 = "1fc6d541eee45f233f786e313090c6fbe23c1f99eebb4f88586852fb044bcb3a"


def require(condition, message):
    if not condition:
        raise RuntimeError("Current governance: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bound(path, expected):
    require(digest(path) == expected, "immutable source evidence: " + path.name)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == ".gz" else data)


def validate(base, updates, evidence, inputs, binaries):
    require(updates["schema"] == 1 and updates["base_matrix_sha256"] == BASE_SHA256
            and updates["runtime_commit"] == base["runtime_commit"]
            and updates["generator_commit"] == base["generator_commit"]
            and updates["whole_goal_complete"] is False, "locked matrix extension")
    require(len(base["requirements"]) == 16 and len(updates["updates"]) == 1,
            "complete original responsibility set and single current update")
    require(Counter(row["status"] for row in base["requirements"])
            == {"reconciled_runtime_contract": 13, "open": 3}, "preserved historical matrix")
    update = updates["updates"][0]
    require(update["id"] == "GP13" and update["status"] == "reconciled_source_contract"
            and update["remaining"] is None, "source provenance obligation only")
    old, ordinary, optimized, milestone = [evidence[name] for name in update["evidence"]]
    require(old["complete"] is True and old["complete_sq02"] is True
            and old["summary"] == {
                "registered_tests": 1101, "python_commands": 716, "comparison_commands": 513,
                "source_control_commands": 203, "primary_source_owners": 64,
                "ordinary_source_control_passes": 203, "optimized_source_control_passes": 201,
                "changed_entry_passes_per_mode": 6, "native_consumers_reused": 707},
            "complete preceding source/comparison qualification")
    require(ordinary["complete"] is True and optimized["complete"] is True
            and ordinary["complete_sq02"] is True and optimized["complete_sq02"] is True
            and ordinary["optimized"] == 0 and optimized["optimized"] == 1
            and ordinary["input_hashes"] == optimized["input_hashes"]
            and ordinary["source_changes"] == optimized["source_changes"]
            and len(ordinary["source_changes"]) == 87
            and len(ordinary["negative_controls"]) == len(optimized["negative_controls"]) == 7
            and all(row["rejected"] is True for value in (ordinary, optimized)
                    for row in value["negative_controls"]), "complete dual-mode current source transitions")
    require(update["current_scope"] == {
        "registered_tests": 1101, "python_commands": 716, "comparison_commands": 513,
        "source_control_commands": 203, "primary_source_owners": 64,
        "unchanged_native_consumers": 707, "admitted_source_changes": 87,
        "historical_compiler_scope": "201 ordinary compiler cases and two selected optimized cases; P0 retains its enforced ordinary boundary"},
        "honest complete provenance and compiler scope")
    require(milestone["complete"] is True and milestone["installed"] is True
            and milestone["whole_goal_complete"] is False
            and milestone["allowed_fields"] == ["G14.evidence", "G14.open_questions"],
            "post-execution metadata-only admission")
    expected = dict(ordinary["input_hashes"])
    require(set(milestone["source_changes"]) == {"runtime:docs/cpp_generator_parity_ledger.json"},
            "single metadata change after current source join")
    for name, row in milestone["source_changes"].items():
        require(expected[name] == row["before"], "exact prior ledger epoch")
        expected[name] = row["after"]
    for path in (UPDATES, Path(__file__)):
        key = "runtime:" + path.relative_to(ROOT).as_posix()
        require(key not in expected, "status extension is not historical execution")
        expected[key] = digest(path)
    require(inputs == expected, "only the declared status data/reader extend current sources")
    require(binaries == ordinary["native_elf_files"] == optimized["native_elf_files"]
            and len(binaries) == 707, "unchanged native consumers")
    current = copy.deepcopy(base)
    gp13 = next(row for row in current["requirements"] if row["id"] == "GP13")
    require(gp13["status"] == "open", "original GP13 status")
    gp13["status"] = update["status"]
    gp13["remaining"] = None
    gp13["qualification"] = update["qualification"]
    gp13["current_evidence"].extend(name for name in update["evidence"]
                                  if name not in gp13["current_evidence"])
    require([row["id"] for row in current["requirements"] if row["status"] == "open"]
            == updates["open_requirements"] == ["GP11", "GP16"], "remaining full metadata/integration obligations")
    require(updates["pending_compatibility_decisions"] == ["option_model", "column_attribute"],
            "no implicit exception approval")
    return current


def controls(base, updates, evidence, inputs, binaries):
    results = []
    for name in ("missing_requirement", "wrong_requirement", "missing_source_join", "wrong_mode",
                 "missing_transition", "unsupported_scope", "unqualified_source", "changed_consumer"):
        b, u, e, i, n = [copy.deepcopy(value) for value in (base, updates, evidence, inputs, binaries)]
        values = list(e.values())
        if name == "missing_requirement":
            b["requirements"].pop()
        elif name == "wrong_requirement":
            u["updates"][0]["id"] = "GP11"
        elif name == "missing_source_join":
            values[1]["complete"] = False
        elif name == "wrong_mode":
            values[2]["optimized"] = 0
        elif name == "missing_transition":
            values[1]["source_changes"].pop(next(iter(values[1]["source_changes"])))
        elif name == "unsupported_scope":
            u["updates"][0]["current_scope"]["historical_compiler_scope"] = "All optimized compilers passed"
        elif name == "unqualified_source":
            i["runtime:scripts/unqualified.py"] = "0" * 64
        elif name == "changed_consumer":
            n[next(iter(n))] = "0" * 64
        try:
            validate(b, u, e, i, n)
        except RuntimeError as error:
            results.append({"case": name, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Governance status corruption accepted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    require(not args.record.exists(), "preserve historical reports")
    updates = bound(UPDATES, UPDATES_SHA256)
    base = bound(ROOT / updates["base_matrix"], BASE_SHA256)
    evidence = {name: bound(ROOT / name, expected)
                for name, expected in updates["updates"][0]["evidence"].items()}
    inputs, refs, binaries = capture_inputs(), references(), native_hashes()
    current = validate(base, updates, evidence, inputs, binaries)
    rejected = controls(base, updates, evidence, inputs, binaries)
    require(capture_inputs() == inputs and references() == refs, "stable source/reference epoch")
    record = {"complete": True, "whole_goal_complete": False,
              "scope": "Current sixteen G17 responsibilities: thirteen runtime and one source contract reconciled; GP11/GP16 remain open",
              "optimized": sys.flags.optimize, "base_sha256": BASE_SHA256,
              "updates_sha256": UPDATES_SHA256, "matrix": current,
              "input_hashes": inputs, "native_elf_files": binaries, "references": refs,
              "controls": rejected, "owned_bytes_removed": 0, "owned_directory_removed": True}
    args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("G17 current requirements: 14 reconciled, GP11/GP16 open; eight corruption controls reject")


if __name__ == "__main__":
    main()
