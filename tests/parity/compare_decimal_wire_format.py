#!/usr/bin/env python3
"""Compare default Decimal serde JSON and Rust's numeric formatting traits."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("decimal_wire_format_5_0_6_cases.json.gz", "Decimal JSON/format"))
