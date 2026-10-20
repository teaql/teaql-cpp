#!/usr/bin/env python3
"""Compare complete query-helper and validation boundaries with source outputs."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('core_query_boundaries_5_0_6_cases.json.gz',
                          'Core query source boundaries 5.0.6'))
