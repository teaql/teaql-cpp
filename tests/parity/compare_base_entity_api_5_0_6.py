#!/usr/bin/env python3
"""Compare legacy base entity operations against replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("base_entity_api_5_0_6_cases.json.gz", "Base entity API 5.0.6"))
