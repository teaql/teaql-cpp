#!/usr/bin/env python3
"""Compare business clock, allocation and service sequencing with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("business_service_baseline_5_0_6_cases.json.gz", "Business clock/service"))
