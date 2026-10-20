#!/usr/bin/env python3
"""Reconcile all pinned runtime root contracts with registered module consumers."""
import argparse
import json
from pathlib import Path
from runtime_root_contracts_5_0_6 import OUTPUT, build

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ctest-json", type=Path, required=True)
    parser.add_argument("--ledger", type=Path, default=OUTPUT,
                        help="Current source/fixture ledger; overrides still require exact source matching")
    args = parser.parse_args()
    module = json.loads(args.ledger.read_text())
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    if module["baseline"] != lock["commit"]:
        raise SystemExit("Module root ledger must use the pinned Rust baseline")
    original = build()
    if module != original:
        raise SystemExit("Module root ledger differs from current committed source or called C++ fixtures")
    expected = {(r["source"], r["name"], r["line"]) for r in original["tests"]}
    actual = {(r["source"], r["name"], r["line"]) for r in module["tests"]}
    if actual != expected or len(actual) != len(module["tests"]):
        raise SystemExit("Missing, duplicated or changed module root contract")
    registered = {r["name"] for r in json.loads(args.ctest_json.read_text())["tests"]}
    for row in module["tests"]:
        if not row["module_checks"] or not set(row["module_checks"]) <= registered:
            raise SystemExit(f"Missing imported root checks: {row['name']}")
    print(f"Verified current source/fixture registration locators for all {len(actual)} runtime root contracts; semantic execution is separate")


if __name__ == "__main__":
    main()
