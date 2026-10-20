#!/usr/bin/env python3
"""Compare Decimal scale and codec results with locked Rust observations."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("decimal_scale_5_0_6_cases.json.gz", "Decimal scale/codec"))
