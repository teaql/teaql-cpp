#!/usr/bin/env python3
"""Compare actual SQLite results after cached HAVING values change."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_cached_partition_protocol_5_0_6_cases.json.gz', 'SQLite cached partition'))
