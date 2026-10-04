#!/usr/bin/env python3
"""Compare SQL log provenance against the pinned data-service implementation."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_log_cases.json", "SQL log provenance"))
