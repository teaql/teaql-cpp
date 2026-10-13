#!/usr/bin/env python3
"""Compare mutation model calls with separately replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("mutation_model_api_5_0_6_cases.json.gz", "Mutation model API 5.0.6"))
