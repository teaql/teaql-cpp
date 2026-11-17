#!/usr/bin/env python3
"""Join the complete named custom-comparison cohort to its source fixtures."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from generated_cli_source_transition_5_0_6 import current_comparator
from inventory_parity_baselines import direct_corpus


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/parity/custom_comparison_binding_manifest_5_0_6.json.gz"
MANIFEST_SHA256 = "63b2e9133c4f55cda0669e9b106aba16d0f52abfae3eaeac4a3a7119ecbf94f0"
RUNTIME = "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
GENERATOR = "45e1572c4956ca963410d8ea436ead3c8857087a"


def require(condition, message):
    if not condition:
        raise RuntimeError("Custom comparison binding: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    value = path.read_bytes()
    return json.loads(gzip.decompress(value) if path.suffix == ".gz" else value)


def properties(test):
    return {row["name"]: row["value"] for row in test["properties"]}


def normalized(command, source, build):
    return ["${PYTHON}", *[
        value.replace(str(build), "${BINARY_DIR}").replace(str(source), "${SOURCE_DIR}")
        for value in command[1:]]]


def member(value, pointer):
    for component in pointer:
        require(isinstance(value, dict) and component in value
                or isinstance(value, list) and type(component) is int
                and 0 <= component < len(value), "declared source member is missing")
        value = value[component]
    return value


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def validate(manifest, entries, build):
    from narrow_integer_registration_source_admission_5_0_6 import separate_custom_registration
    entries, narrow_registration = separate_custom_registration(entries, build)
    require(manifest["complete"] is True and manifest["whole_goal_complete"] is False
            and manifest["complete_sq02"] is False
            and manifest["runtime_commit"] == RUNTIME and manifest["generator_commit"] == GENERATOR,
            "locked baseline and bounded qualification required")
    rows, groups = manifest["rows"], manifest["groups"]
    require(len(rows) == len({row["test"] for row in rows}) == 160
            and len(groups) == len({row["cohort"] for row in rows}) == 35
            and sum(len(row["members"]) for row in rows) == 180,
            "complete custom command/cohort/member inventory required")
    tests = {entry["name"]: entry for entry in entries}
    require(len(tests) == len(entries), "duplicate CTest name")
    candidates = set()
    for test in entries:
        command = test["command"]
        if len(command) < 2:
            continue
        script = Path(command[1])
        if script.parent != ROOT / "tests/parity":
            continue
        if (script.name.startswith(("compare_", "diagnose_memory_")) and direct_corpus(script) is None
                or script.name == "run_clock_boundary_5_0_6.py"):
            candidates.add(test["name"])
    require(candidates == {row["test"] for row in rows},
            "unqualified or missing immediate-directory custom comparison")
    require(digest(ROOT / manifest["locator_path"]) == manifest["locator_sha256"]
            and digest(ROOT / "scripts/generated_cli_source_transition_5_0_6.py")
            == manifest["transition_helper_sha256"], "historical locator/transition identity changed")
    prior = {row["test"]: row for row in read(ROOT / manifest["locator_path"])["rows"]
             if row.get("comparison")}
    source_data = {}
    for key, group in groups.items():
        path = ROOT / group["contracts"]
        require(digest(path) == group["contracts_sha256"], "source contract identity changed")
        source_data[key] = read(path)
        require(source_data[key]["runtime_commit"] == RUNTIME
                and source_data[key].get("generator_commit", GENERATOR) == GENERATOR,
                "source contract baseline changed")
        owner = tests[group["source_test"]]
        require([test["name"] for test in entries
                 if group["fixture"] in properties(test).get("FIXTURES_SETUP", [])]
                == [owner["name"]], "unique source fixture setup required")
        require(Path(owner["command"][0]).resolve() == Path(sys.executable).resolve()
                and normalized(owner["command"], ROOT, build) == group["source_command"],
                "source validator command changed")
        for name, wanted in group["source_inputs"].items():
            require(digest(ROOT / name) == wanted, "qualified source helper changed: " + name)
        for evidence in group["evidence"]:
            require(digest(ROOT / evidence["path"]) == evidence["sha256"],
                    "retained source evidence changed")
    for row in rows:
        require(row["test"] in tests and row["test"] in prior, "missing custom registration")
        test = tests[row["test"]]
        previous = prior[row["test"]]
        previous_root = Path(previous["command"][1]).parents[2]
        expected = normalized(previous["command"], previous_root, previous_root / "build_native_current")
        active = current_comparator(ROOT, previous["script"])
        expected[1] = "${SOURCE_DIR}/" + active
        require(row["cohort"] == previous["source_contract"]
                and row["historical_script"] == previous["script"]
                and row["script"] == active and row["command"] == expected
                and Path(test["command"][0]).resolve() == Path(sys.executable).resolve()
                and normalized(test["command"], ROOT, build) == expected,
                "unqualified comparator, native target or argument selection")
        require(digest(ROOT / row["script"]) == row["script_sha256"],
                "current comparison/projection source changed")
        group = groups[row["cohort"]]
        require(group["fixture"] in row["required_fixtures"]
                and set(row["required_fixtures"]) <= set(properties(test).get("FIXTURES_REQUIRED", [])),
                "required source/recipe fixture missing")
        selected = row["members"]
        require(bool(selected) and len({json.dumps(value["pointer"]) for value in selected}) == len(selected),
                "complete unique source member selections required")
        for selected_member in selected:
            require(fingerprint(member(source_data[row["cohort"]], selected_member["pointer"]))
                    == selected_member["value_sha256"], "source selection/projection identity changed")
    return [*rows, narrow_registration]


def controls(manifest, tests, build):
    results = []
    for case in ("missing_entry", "duplicate_entry", "wrong_command", "historical_script",
                 "missing_source_fixture", "missing_source_setup", "wrong_source_command",
                 "wrong_baseline", "wrong_contract_hash", "missing_member", "wrong_member_hash",
                 "duplicate_member", "unknown_comparison"):
        changed = copy.deepcopy(manifest)
        entries = copy.deepcopy(tests)
        selected = changed["rows"][0]
        by_name = {test["name"]: test for test in entries}
        test = by_name[selected["test"]]
        group = changed["groups"][selected["cohort"]]
        owner = by_name[group["source_test"]]
        if case == "missing_entry":
            changed["rows"].pop()
        elif case == "duplicate_entry":
            changed["rows"][-1] = copy.deepcopy(selected)
        elif case == "wrong_command":
            test["command"].extend(["--case", "controlled_wrong_case"])
        elif case == "historical_script":
            name = "generated_expression_diagnostics_rust_parity"
            row = next(row for row in changed["rows"] if row["test"] == name)
            by_name[name]["command"][1] = str(ROOT / row["historical_script"])
        elif case == "missing_source_fixture":
            test["properties"] = [row for row in test["properties"] if row["name"] != "FIXTURES_REQUIRED"]
        elif case == "missing_source_setup":
            owner["properties"] = [row for row in owner["properties"] if row["name"] != "FIXTURES_SETUP"]
        elif case == "wrong_source_command":
            owner["command"].append("--controlled_wrong_source")
        elif case == "wrong_baseline":
            changed["runtime_commit"] = "0" * 40
        elif case == "wrong_contract_hash":
            group["contracts_sha256"] = "0" * 64
        elif case == "missing_member":
            selected["members"][0]["pointer"] = ["controlled_missing_member"]
        elif case == "wrong_member_hash":
            selected["members"][0]["value_sha256"] = "0" * 64
        elif case == "duplicate_member":
            selected["members"][-1] = copy.deepcopy(selected["members"][0])
        elif case == "unknown_comparison":
            extra = copy.deepcopy(test)
            extra["name"] = "controlled_unqualified_comparison"
            entries.append(extra)
        try:
            validate(changed, entries, build)
        except RuntimeError as error:
            results.append({"case": case, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Custom comparison corruption accepted: " + case)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "preserve custom binding evidence")
    require(digest(MANIFEST) == MANIFEST_SHA256, "custom binding manifest changed")
    manifest = read(MANIFEST)
    build = args.build_dir.resolve()
    entries = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"], text=True))["tests"]
    rows = validate(manifest, entries, build)
    results = controls(manifest, entries, build)
    if args.record:
        record = {"complete": True, "whole_goal_complete": False, "complete_sq02": False,
                  "scope": "Exact current custom-command/source-member/fixture join; source and native executions need their separate CTest gates",
                  "manifest_sha256": MANIFEST_SHA256, "runtime_commit": RUNTIME,
                  "generator_commit": GENERATOR, "optimized": sys.flags.optimize,
                  "registered_tests": len(entries), "rows": rows, "controls": results}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("161 custom comparisons: 160 historical comparisons / 35 source cohorts / 180 source-member selections, "
          "plus the complete 376-case narrow-integer source/receipt/fixture binding; "
          "thirteen historical controls reject; source/native execution and complete SQ02 remain separate")


if __name__ == "__main__":
    main()
