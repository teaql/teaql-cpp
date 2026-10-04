#!/usr/bin/env python3
"""Compare the runtime Business ID profiles against pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("runtime_business_id_cases.json", "Runtime Business ID profiles"))
