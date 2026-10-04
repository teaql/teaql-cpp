#!/usr/bin/env python3
"""Compare Web/Xls JSON models with actual pinned Rust serde implementations."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("presentation_cases.json", "Presentation"))
