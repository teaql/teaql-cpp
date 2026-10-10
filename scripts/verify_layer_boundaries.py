#!/usr/bin/env python3
"""Check compiled runtime symbols and generated CMake kernel dependencies."""
import argparse
import re
import subprocess
from pathlib import Path


EXPECTED = {
    "teaql_core": {"teaql::memory"},
    "teaql_memory": {"Threads::Threads"},
    "teaql_data_service": {"teaql::core", "teaql::async", "Threads::Threads"},
    "teaql_sql": {"teaql::data_service"},
    "teaql_runtime": {"teaql::sql"},
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", required=True, type=Path)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--metadata", type=Path)
    args = parser.parse_args()
    symbols = subprocess.run([args.nm, "-u", str(args.runtime)],
                             text=True, capture_output=True, check=True).stdout
    native = sorted(set(re.findall(r"\bsqlite3_\w+\b", symbols)))
    if native:
        raise RuntimeError("Runtime calls the SQLite C API: " + ", ".join(native))
    if args.metadata:
        sections = {}
        current = None
        for line in args.metadata.read_text().splitlines():
            if line.startswith("[") and line.endswith("]"):
                current = line[1:-1]
                sections[current] = {}
            elif line and current:
                key, value = line.split("=", 1)
                sections[current][key] = value
        if sections.keys() != EXPECTED.keys():
            raise RuntimeError("Incomplete generated kernel dependency metadata")
        for target, allowed in EXPECTED.items():
            for property_name in ["LINK", "INTERFACE"]:
                value = sections[target][property_name]
                if re.search(r"sqlite3|legacy_sqlite|provider_sqlite", value, re.I):
                    raise RuntimeError(f"{target} retains a backend dependency: {value}")
                dependencies = set(filter(None, value.split(";")))
                if dependencies != allowed:
                    raise RuntimeError(f"Unreviewed {target} {property_name}: {value}")
            if re.search(r"(?:^|[/;])(?:sqlite|legacy_sqlite_compiler)\.cpp(?:;|$)",
                         sections[target]["SOURCES"]):
                raise RuntimeError(f"{target} compiles the compatibility SQLite path")
        print("Verified kernel layers retain their independent dependencies")
    print("Verified compiled runtime has no SQLite C API calls")


if __name__ == "__main__":
    main()
