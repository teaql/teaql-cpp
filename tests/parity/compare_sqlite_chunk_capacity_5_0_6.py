#!/usr/bin/env python3
"""Compare actual native cursor capacity, complete ordered IDs and cleanup."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_chunk_capacity_5_0_6_cases.json.gz", "SQLite cursor capacities"))
