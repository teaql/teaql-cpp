#!/usr/bin/env python3
"""Compare primitive Decimal conversions with the locked Rust implementation."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("decimal_numeric_cases.json", "Decimal numeric conversion"))
