#!/usr/bin/env python3
"""Compare transaction mutation/readback behavior with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_transaction_protocol_5_0_6_cases.json.gz", "SQL transaction execution"))
