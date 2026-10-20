#!/usr/bin/env python3
"""Compare structured SQLite driver/provider models against actual original calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_error_model_5_0_6_cases.json.gz", "SQLite driver/provider error models"))
