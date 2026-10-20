#!/usr/bin/env python3
"""Compare binding order, policies, original LIKE operands and rebinding."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_bindings_source_5_0_6_cases.json.gz', 'SQL binding source'))
