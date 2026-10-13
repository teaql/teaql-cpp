#!/usr/bin/env python3
"""Compare check model against actual replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("check_model_5_0_6_cases.json.gz", "check model 5.0.6"))
