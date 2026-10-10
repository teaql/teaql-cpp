#!/usr/bin/env python3
"""Compare extended Decimal arithmetic, raw scale/sign and panic boundaries."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('decimal_extended_cases.json.gz', 'Extended Decimal'))
