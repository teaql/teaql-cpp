#!/usr/bin/env python3
"""Compare exact/radix parsing with the locked Rust implementation."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("decimal_parsing_cases.json", "Decimal parsing"))
