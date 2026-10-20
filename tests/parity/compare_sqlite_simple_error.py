#!/usr/bin/env python3
"""Compare enabled scalar/text/unit driver payloads against actual original calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_simple_error_5_0_6_cases.json.gz", "SQLite scalar/text/unit errors"))
