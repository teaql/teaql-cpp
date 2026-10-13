#!/usr/bin/env python3
"""Compare full readonly schema hash and diagnostics against actual Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("readonly_schema_hash_5_0_6_cases.json.gz", "Readonly schema hash 5.0.6"))
