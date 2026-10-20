#!/usr/bin/env python3
"""Compare native64 SQLite literals, lexical accounting and complete error payloads."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_types_boundaries_5_0_6_cases.json.gz', 'SQL types source boundaries'))
