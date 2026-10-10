#!/usr/bin/env python3
"""Compare full-range SystemTime arithmetic and execution metadata with Rust."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('system_time_cases.json.gz', 'SystemTime'))
