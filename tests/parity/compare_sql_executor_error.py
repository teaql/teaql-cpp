#!/usr/bin/env python3
"""Compare complete pinned executor-error Debug, Display and absent source."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_executor_error_cases.json', 'SQL executor errors'))
