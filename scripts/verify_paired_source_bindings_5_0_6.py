#!/usr/bin/env python3
"""Check that every source cohort also retains its actual native execution entry."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

from verify_custom_comparison_bindings_5_0_6 import (
    MANIFEST, MANIFEST_SHA256, ROOT, digest, normalized, properties, read, require,
)


CTEST_ARGUMENT_COHORTS = {
    "tests/parity/logging_cli_contract_5_0_6.json",
    "tests/parity/runtime_examples_cli_contract_5_0_6.json",
    "tests/parity/sqlite_raw_debug_cli_contract_5_0_6.json",
    "tests/parity/sqlite_transaction_cli_contract_5_0_6.json",
}


def expected_command(key, group):
    command = ["${PYTHON}", group["source_command"][1]]
    if key in ("query_cli", "dynamic_json_cli"):
        command.extend(["--contracts", "${SOURCE_DIR}/" + group["contracts"]])
    if key == "custom_public_cli":
        for flag, target in (("--cpp-transaction", "teaql_transaction_scope_intent_probe"),
                             ("--cpp-cache", "teaql_initialized_snapshot_cache_probe"),
                             ("--cpp-deferred", "teaql_deferred_facet_page_probe")):
            command.extend([flag, "${BINARY_DIR}/" + target])
    else:
        command.extend(["--cpp-build-dir", "${BINARY_DIR}"])
        if key in CTEST_ARGUMENT_COHORTS:
            command.extend(["--ctest-build-dir", "${BINARY_DIR}"])
    return command


def validate(manifest, tests, build):
    require(len(manifest["groups"]) == 35, "complete paired source cohort required")
    names = {test["name"] for test in tests}
    require(len(names) == len(tests), "duplicate paired CTest name")
    rows = []
    for key, group in manifest["groups"].items():
        owners = [test for test in tests if group["fixture"]
                  in properties(test).get("FIXTURES_SETUP", [])]
        require(len(owners) == 1 and owners[0]["name"] == group["source_test"]
                and normalized(owners[0]["command"], ROOT, build) == group["source_command"],
                "paired source prerequisite changed")
        expected = expected_command(key, group)
        peers = [test for test in tests if test["name"].endswith("_source_rust_parity")
                 and len(test["command"]) > 1
                 and test["command"][1] == owners[0]["command"][1]
                 and group["fixture"] in properties(test).get("FIXTURES_REQUIRED", [])]
        require(len(peers) == 1, "unique native peer missing for " + key)
        test = peers[0]
        require(Path(test["command"][0]).resolve() == Path(sys.executable).resolve()
                and normalized(test["command"], ROOT, build) == expected
                and "--verify-only" not in test["command"],
                "native peer executable, contract or execution mode changed")
        rows.append({"cohort": key, "source_test": group["source_test"],
                     "test": test["name"], "fixture": group["fixture"],
                     "contracts": group["contracts"], "contracts_sha256": group["contracts_sha256"],
                     "source_command": group["source_command"], "native_command": expected,
                     "script_sha256": digest(Path(test["command"][1]))})
    require(len(rows) == len({row["test"] for row in rows}) == 35,
            "complete unique source/native pairs required")
    scripts = {group["source_command"][1].replace("${SOURCE_DIR}", str(ROOT))
               for group in manifest["groups"].values()}
    actual = {test["name"] for test in tests if len(test["command"]) > 1
              and test["command"][1] in scripts and "--verify-only" not in test["command"]}
    require(actual == {row["test"] for row in rows}, "unqualified source/native dispatcher")
    return rows


def controls(manifest, tests, build, rows):
    results = []
    for case in ("missing_native", "duplicate_native", "source_only_mode", "wrong_build",
                 "missing_source_requirement", "missing_setup", "wrong_contract", "unknown_peer"):
        entries = copy.deepcopy(tests)
        first = next(test for test in entries if test["name"] == rows[0]["test"])
        if case == "missing_native":
            entries.remove(first)
        elif case == "duplicate_native":
            entries.append(copy.deepcopy(first))
        elif case == "source_only_mode":
            first["command"].append("--verify-only")
        elif case == "wrong_build":
            first["command"] = [value.replace(str(build), "/controlled/wrong_build")
                                for value in first["command"]]
        elif case == "missing_source_requirement":
            first["properties"] = [row for row in first["properties"]
                                   if row["name"] != "FIXTURES_REQUIRED"]
        elif case == "missing_setup":
            owner = next(test for test in entries if test["name"] == rows[0]["source_test"])
            owner["properties"] = [row for row in owner["properties"]
                                   if row["name"] != "FIXTURES_SETUP"]
        elif case == "wrong_contract":
            selected = next(row for row in rows if row["cohort"] == "query_cli")
            peer = next(test for test in entries if test["name"] == selected["test"])
            peer["command"][peer["command"].index("--contracts") + 1] = str(
                ROOT / manifest["groups"]["dynamic_json_cli"]["contracts"])
        elif case == "unknown_peer":
            extra = copy.deepcopy(first)
            extra["name"] = "controlled_extra_source_rust_parity"
            entries.append(extra)
        try:
            validate(manifest, entries, build)
        except RuntimeError as error:
            results.append({"case": case, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Paired source/native corruption accepted: " + case)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "preserve paired source evidence")
    require(digest(MANIFEST) == MANIFEST_SHA256, "source cohort manifest changed")
    manifest = read(MANIFEST)
    build = args.build_dir.resolve()
    tests = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"], text=True))["tests"]
    rows = validate(manifest, tests, build)
    results = controls(manifest, tests, build, rows)
    if args.record:
        record = {"complete": True, "whole_goal_complete": False, "complete_sq02": False,
                  "scope": "All current paired source/native command and execution-mode bindings; actual source/native execution remains a separate gate",
                  "runtime_commit": manifest["runtime_commit"], "generator_commit": manifest["generator_commit"],
                  "manifest_sha256": MANIFEST_SHA256, "optimized": sys.flags.optimize,
                  "registered_tests": len(tests), "rows": rows, "controls": results}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("35 exact source/native execution pairs; eight missing/mode/target/contract controls reject; "
          "actual execution and complete SQ02 remain separate")


if __name__ == "__main__":
    main()
