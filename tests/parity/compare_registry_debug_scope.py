#!/usr/bin/env python3
"""Compare original registry derives, ordering and independent snapshots."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('registry_debug_scope_cases.json.gz', 'Registry diagnostics'))
