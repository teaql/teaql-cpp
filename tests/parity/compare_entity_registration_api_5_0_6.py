#!/usr/bin/env python3
"""Compare entity registration dispatch against replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("entity_registration_api_5_0_6_cases.json.gz", "Entity registration 5.0.6"))
