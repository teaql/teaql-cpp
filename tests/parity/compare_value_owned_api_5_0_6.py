#!/usr/bin/env python3
"""Compare owned Value conversions with separately replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("value_owned_api_5_0_6_cases.json.gz", "Owned Value API 5.0.6"))
