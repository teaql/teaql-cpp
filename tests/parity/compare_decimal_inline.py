#!/usr/bin/env python3
"""Compare original inline Decimal characters and fixed-buffer failures."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('decimal_inline_5_0_6_cases.json.gz', 'Decimal inline string'))
