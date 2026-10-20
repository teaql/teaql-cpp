#!/usr/bin/env python3
"""Compare complete Expr factory and threshold boundaries with locked Rust."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('core_expression_boundaries_5_0_6_cases.json.gz', 'Core Expr boundaries'))
