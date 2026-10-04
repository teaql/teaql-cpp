#!/usr/bin/env python3
"""Compare structured checks and runtime errors against the pinned Rust runtime."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("runtime_check_cases.json", "Runtime checks/errors"))
