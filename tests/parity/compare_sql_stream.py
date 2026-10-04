#!/usr/bin/env python3
"""Compare native and buffered SQL streams with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_stream_cases.json", "SQL stream execution"))
