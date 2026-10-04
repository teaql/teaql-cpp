#!/usr/bin/env python3
"""Compare Context registration and locals against the pinned Rust runtime."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("runtime_context_cases.json", "Runtime Context resources"))
