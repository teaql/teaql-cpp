#!/usr/bin/env python3
"""Compare complete request model Debug, conversion snapshots and generic capabilities."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('request_api_cases.json.gz', 'Request API'))
