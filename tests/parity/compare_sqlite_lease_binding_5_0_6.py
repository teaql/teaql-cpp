#!/usr/bin/env python3
"""Compare actual provider lease-before-binding schedules and errors."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_lease_binding_5_0_6_cases.json.gz", "SQLite lease/binding"))
