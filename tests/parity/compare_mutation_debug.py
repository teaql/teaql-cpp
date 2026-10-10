#!/usr/bin/env python3
"""Compare full mutation roles/commands with original derived Debug formatting."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('mutation_debug_cases.json.gz', 'Mutation Debug'))
