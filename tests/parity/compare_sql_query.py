#!/usr/bin/env python3
"""Compare query executor caching, routing and diagnostics with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_query_protocol_5_0_6_cases.json.gz", "SQL query executor"))
