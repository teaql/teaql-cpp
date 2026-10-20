#!/usr/bin/env python3
"""Compare native driver diagnostic payloads with actual original calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_native_diagnostics_5_0_6_cases.json.gz", "SQLite native diagnostic payloads"))
