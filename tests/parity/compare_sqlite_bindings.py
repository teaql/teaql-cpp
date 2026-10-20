#!/usr/bin/env python3
"""Compare SQLite binding/storage with the pinned provider's public execute."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sqlite_bindings_5_0_6_cases.json.gz", "SQLite bindings/storage"))
