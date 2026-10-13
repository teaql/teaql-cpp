#!/usr/bin/env python3
"""Compare original Rust 5.0.6 mutation execution, complete traces and privacy."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_execution_5_0_6_cases.json.gz', 'Rust 5.0.6 SQL mutation execution'))
