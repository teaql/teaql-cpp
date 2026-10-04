#!/usr/bin/env python3
"""Compare the core Business ID contracts with fixed Rust observations."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("business_id_cases.json", "Business ID core"))
