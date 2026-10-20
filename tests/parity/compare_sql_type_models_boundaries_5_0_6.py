#!/usr/bin/env python3
"""Compare native64 SQL model/error defaults, derives, comments and usize maxima."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_type_models_boundaries_5_0_6_cases.json.gz', 'SQL type model boundaries'))
