#!/usr/bin/env python3
"""Compare sql log against actual replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_log_5_0_6_cases.json.gz", "sql log 5.0.6"))
