#!/usr/bin/env python3
"""Compare non-Clone projection wrappers against original Rust observations."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_projection_scope_cases.json", "SQL projection scope"))
