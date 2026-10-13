#!/usr/bin/env python3
"""Compare public CompactRow calls with separately replayed Rust 5.0.6 evidence."""

from compare_values import main


if __name__ == "__main__":
    raise SystemExit(main("compact_row_api_5_0_6_cases.json.gz", "CompactRow API 5.0.6"))
