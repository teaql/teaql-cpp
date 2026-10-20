#!/usr/bin/env python3
"""Compare Decimal results including scale, sign and 96-bit coefficient."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("decimal_5_0_6_cases.json.gz", "Decimal arithmetic"))
