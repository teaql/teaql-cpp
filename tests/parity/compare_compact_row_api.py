#!/usr/bin/env python3
"""Compare actual original CompactRow public calls and immutable layout snapshots."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("compact_row_api_cases.json.gz", "CompactRow API"))
