#!/usr/bin/env python3
"""Compare schema sequencing and dynamic bootstrap scopes against pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("schema_runtime_baseline_5_0_6_cases.json.gz", "Schema runtime"))
