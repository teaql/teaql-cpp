#!/usr/bin/env python3
"""Compare the complete SQL model/diagnostic observations with original Rust."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_model_api_cases.json.gz', 'SQL model API'))
