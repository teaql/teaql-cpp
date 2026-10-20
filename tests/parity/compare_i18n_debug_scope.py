#!/usr/bin/env python3
"""Compare original derived diagnostics and cloned catalog private state."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('i18n_debug_scope_baseline_5_0_6_cases.json.gz', 'Catalog diagnostics'))
