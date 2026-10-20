#!/usr/bin/env python3
"""Compare native lock poisoning and synchronous binding precedence."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_lease_binding_guard_5_0_6_cases.json.gz", "SQLite binding guards"))
