#!/usr/bin/env python3
"""Compare generic SqlDialect query/expression/schema behavior with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_dialect_cases.json", "SQL dialect query/schema"))
