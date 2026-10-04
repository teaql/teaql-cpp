#!/usr/bin/env python3
"""Compare transaction mutation/readback behavior with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_transaction_cases.json", "SQL transaction execution"))
