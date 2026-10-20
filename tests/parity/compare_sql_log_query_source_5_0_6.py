#!/usr/bin/env python3
"""Compare default-provider discovery and field policy with actual source outputs."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_log_query_source_5_0_6_cases.json.gz',
                         'SQL-log query/source policy'))
