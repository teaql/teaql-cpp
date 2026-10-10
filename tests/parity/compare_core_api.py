#!/usr/bin/env python3
"""Compare actual Rust Timestamp traits and dynamic metadata builder calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("core_api_cases.json", "Core API"))
