#!/usr/bin/env python3
"""Compare data-service model and default diagnostics with original Rust."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('data_service_model_cases.json.gz', 'Data-service models'))
