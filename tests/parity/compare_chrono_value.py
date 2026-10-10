#!/usr/bin/env python3
"""Compare native UTC Value conversion with original chrono-backed calls."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('chrono_value_cases.json', 'UTC Value conversion'))
