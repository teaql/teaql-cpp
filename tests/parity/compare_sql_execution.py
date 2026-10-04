#!/usr/bin/env python3
"""Compare mutation execution and diagnostic journals with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_execution_cases.json", "SQL mutation execution"))
