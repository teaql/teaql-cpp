#!/usr/bin/env python3
"""Compare the runtime Business ID profiles against pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("runtime_business_id_baseline_5_0_6_cases.json.gz", "Runtime Business ID profiles"))
