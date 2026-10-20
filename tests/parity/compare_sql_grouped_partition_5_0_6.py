#!/usr/bin/env python3
"""Compare grouped, filtered per-owner ranking through actual SQLite execution."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_grouped_partition_5_0_6_cases.json.gz', 'SQLite grouped partition execution'))
