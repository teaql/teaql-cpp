#!/usr/bin/env python3
"""Compare structured checks and runtime errors against the pinned Rust runtime."""
from compare_runtime_error_debug import main
if __name__ == "__main__":
    raise SystemExit(main("runtime_check_baseline_5_0_6_cases.json.gz", "Runtime checks/errors"))
