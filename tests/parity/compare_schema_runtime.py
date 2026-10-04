#!/usr/bin/env python3
"""Compare schema sequencing and dynamic bootstrap scopes against pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("schema_runtime_cases.json", "Schema runtime"))
