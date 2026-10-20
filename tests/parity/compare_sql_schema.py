#!/usr/bin/env python3
"""Compare schema indexes, identifiers and foreign-key naming with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_schema_5_0_6_cases.json.gz", "SQL schema indexes/naming"))
