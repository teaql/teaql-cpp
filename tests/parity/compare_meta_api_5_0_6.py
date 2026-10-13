#!/usr/bin/env python3
"""Compare descriptor metadata with separately replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("meta_api_5_0_6_cases.json.gz", "Metadata API 5.0.6"))
