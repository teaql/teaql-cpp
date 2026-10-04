#!/usr/bin/env python3
"""Compare business clock, allocation and service sequencing with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("business_service_cases.json", "Business clock/service"))
