#!/usr/bin/env python3
"""Compare original per-invocation mutation backend capture and kind calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_executor_kind_cases.json", "SQL executor backend capture"))
