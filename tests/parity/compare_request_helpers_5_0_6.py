#!/usr/bin/env python3
"""Compare request helpers against actual replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("request_helpers_5_0_6_cases.json.gz", "request helpers 5.0.6"))
