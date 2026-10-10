#!/usr/bin/env python3
"""Compare original core business/search and Decimal helper diagnostics."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('core_aux_debug_cases.json.gz', 'Auxiliary core diagnostics'))
