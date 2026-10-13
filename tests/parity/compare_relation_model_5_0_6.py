#!/usr/bin/env python3
"""Compare relation model against actual replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("relation_model_5_0_6_cases.json.gz", "relation model 5.0.6"))
