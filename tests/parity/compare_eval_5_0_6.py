#!/usr/bin/env python3
"""Compare eval/load-state calls against replayed Rust 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("eval_5_0_6_cases.json.gz", "Eval protocol 5.0.6"))
