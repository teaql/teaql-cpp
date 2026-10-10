#!/usr/bin/env python3
"""Compare original SmartList public calls, metadata and recursive facets."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("smart_list_api_cases.json.gz", "SmartList API"))
