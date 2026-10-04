#!/usr/bin/env python3
"""Compare internal ID and Context behavior against pinned Rust observations."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("internal_id_cases.json", "Internal ID"))
