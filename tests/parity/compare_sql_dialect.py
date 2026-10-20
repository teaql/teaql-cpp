#!/usr/bin/env python3
"""Compare generic SqlDialect query/expression/schema behavior with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_dialect_5_0_6_cases.json.gz", "SQL dialect query/schema"))
