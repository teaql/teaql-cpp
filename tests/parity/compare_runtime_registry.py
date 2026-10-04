#!/usr/bin/env python3
"""Compare runtime metadata and registry semantics against pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("runtime_registry_cases.json", "Runtime metadata/registry"))
