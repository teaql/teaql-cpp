#!/usr/bin/env python3
"""Compare core public traits against separately replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("core_api_5_0_6_cases.json.gz", "Core API 5.0.6"))
