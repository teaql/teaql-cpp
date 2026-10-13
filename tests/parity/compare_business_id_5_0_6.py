#!/usr/bin/env python3
"""Compare business ID core contracts against replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("business_id_5_0_6_cases.json.gz", "Business ID protocol 5.0.6"))
