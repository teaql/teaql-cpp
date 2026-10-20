#!/usr/bin/env python3
"""Compare actual Timestamp methods, Debug flags and raw JSON acceptance."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('timestamp_api_baseline_5_0_6_cases.json.gz', 'Timestamp API'))
