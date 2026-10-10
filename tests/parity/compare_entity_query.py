#!/usr/bin/env python3
"""Compare public EntityDataService query orchestration with pinned Rust calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("entity_query_cases.json", "Entity query execution"))
