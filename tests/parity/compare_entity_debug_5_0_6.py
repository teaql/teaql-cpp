#!/usr/bin/env python3
"""Compare entity structural diagnostics against replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("entity_debug_5_0_6_cases.json.gz", "Entity diagnostics 5.0.6"))
