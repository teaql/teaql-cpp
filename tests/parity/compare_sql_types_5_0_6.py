#!/usr/bin/env python3
"""Compare sql types against actual replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_types_5_0_6_cases.json.gz", "sql types 5.0.6"))
