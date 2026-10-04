#!/usr/bin/env python3
"""Compare production SQLite transaction behavior with the pinned provider."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sqlite_transaction_cases.json", "Production SQLite transaction"))
