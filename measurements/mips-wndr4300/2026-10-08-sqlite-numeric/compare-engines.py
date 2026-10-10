#!/usr/bin/env python3
"""Reproduce every recorded numeric discrepancy through SQLite alone."""
import argparse
import decimal
import json
from pathlib import Path
import struct
import subprocess


def bits(value):
    return struct.pack(">d", value).hex()


def observe(executable, emulator, setup, query):
    result = subprocess.run(
        [*emulator, executable, setup, query],
        check=True, capture_output=True, text=True,
    )
    raw = result.stdout.strip()
    return raw.rsplit("bits=", 1)[1], raw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--old", required=True)
    parser.add_argument("--new", required=True)
    parser.add_argument("--emulator", default="/usr/bin/qemu-mips;-cpu;74Kf")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    record = json.loads((root / "tests/parity/sqlite_mips_engine_differences.json").read_text())
    emulator = args.emulator.split(";") if args.emulator else []
    observations = []
    for group in record["groups"]:
        for case in group["cases"]:
            input_value = case["input"]
            if group["name"] == "bindings":
                assert len(input_value["params"]) == 1
                parameter = input_value["params"][0]
                assert parameter["kind"] == "decimal"
                declaration = input_value["affinity"]
                text = format(decimal.Decimal(parameter["value"]), "f")
                value = "'" + text.replace("'", "''") + "'"
                expected = case["rust_bundled"]["stored"]["value"]
                previous = case["cpp_mips_sqlite_3_40_1"]["stored"]["value"]
            else:
                declaration = input_value["decl"]
                value = input_value["sql_value"]
                expected = case["rust_bundled"]["rows"][0]["values"][0]["value"]
                previous = case["cpp_mips_sqlite_3_40_1"]["rows"][0]["values"][0]["value"]
            setup = (
                'CREATE TABLE data(v "' + declaration.replace('"', '""')
                + '"); INSERT INTO data VALUES(' + value + ');'
            )
            query = input_value.get("query", "SELECT v FROM data")
            old_bits, old_text = observe(args.old, emulator, setup, query)
            new_bits, new_text = observe(args.new, emulator, setup, query)
            observations.append({
                "group": group["name"], "case_index": case["case_index"],
                "setup": setup, "query": query,
                "rust_expected_bits": bits(expected),
                "previous_cpp_bits": bits(previous),
                "old_sqlite": old_text, "new_sqlite": new_text,
                "old_reproduces_previous": old_bits == bits(previous),
                "new_matches_original": new_bits == bits(expected),
            })
    print(json.dumps({"observations": observations}, indent=2))
    return int(not all(
        row["old_reproduces_previous"] and row["new_matches_original"]
        for row in observations
    ))


if __name__ == "__main__":
    raise SystemExit(main())
