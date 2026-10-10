#!/usr/bin/env python3
"""Compare all public request helper outcomes and exact original diagnostics."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('request_helpers_cases.json.gz', 'Request helpers'))
