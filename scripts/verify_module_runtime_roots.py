#!/usr/bin/env python3
"""Reconcile all pinned runtime root contracts with registered module consumers."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ctest-json", type=Path, required=True)
    args = parser.parse_args()
    original = json.loads((ROOT / "docs/parity/source_test_dispositions.json").read_text())
    module = json.loads((ROOT / "docs/module_runtime_root_contracts.json").read_text())
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    if module["baseline"] != lock["commit"]:
        raise SystemExit("Module root ledger must use the pinned Rust baseline")
    expected = {(r["source"], r["name"], r["line"]) for r in original["tests"]
                if r["source"] == "teaql-runtime/src/lib.rs"}
    actual = {(r["source"], r["name"], r["line"]) for r in module["tests"]}
    if actual != expected or len(actual) != len(module["tests"]):
        raise SystemExit("Missing, duplicated or changed module root contract")
    registered = {r["name"] for r in json.loads(args.ctest_json.read_text())["tests"]}
    for row in module["tests"]:
        if not row["module_checks"] or not set(row["module_checks"]) <= registered:
            raise SystemExit(f"Missing imported root checks: {row['name']}")
    print(f"Verified registration locators for all {len(actual)} original runtime root contracts")


if __name__ == "__main__":
    main()
