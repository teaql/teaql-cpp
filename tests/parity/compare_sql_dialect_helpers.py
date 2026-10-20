#!/usr/bin/env python3
"""Compare original public SQL quoting and storage-coverage helpers."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_dialect_helper_5_0_6_cases.json.gz", "SQL dialect helpers"))
