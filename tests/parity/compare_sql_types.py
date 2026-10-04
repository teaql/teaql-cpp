#!/usr/bin/env python3
"""Compare SQL models and SQLite diagnostic rendering with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_types_cases.json", "SQL models/rendering"))
