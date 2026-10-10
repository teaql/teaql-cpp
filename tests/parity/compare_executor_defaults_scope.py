#!/usr/bin/env python3
"""Compare actual inherited executor dispatch and untouched observers."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('executor_defaults_scope_cases.json', 'Executor defaults'))
