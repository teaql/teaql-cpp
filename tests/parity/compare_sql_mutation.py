#!/usr/bin/env python3
"""Compare generic mutation compilation with checked-build pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_mutation_5_0_6_cases.json.gz", "SQL mutation compilation"))
