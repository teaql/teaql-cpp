#!/usr/bin/env python3
"""Compare request api against actual replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("request_api_5_0_6_cases.json.gz", "request api 5.0.6"))
