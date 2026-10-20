#!/usr/bin/env python3
"""Compare mutation default provider-hook ordering with original Rust calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_dialect_hook_5_0_6_cases.json.gz", "SQL dialect hook order"))
