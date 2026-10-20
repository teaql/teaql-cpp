#!/usr/bin/env python3
"""Compare original per-invocation mutation backend capture and kind calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_executor_kind_protocol_5_0_6_cases.json.gz", "SQL executor backend capture"))
