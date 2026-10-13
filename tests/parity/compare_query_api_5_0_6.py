#!/usr/bin/env python3
"""Compare query construction with separately replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("query_api_5_0_6_cases.json.gz", "Query API 5.0.6"))
