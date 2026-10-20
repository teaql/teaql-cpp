#!/usr/bin/env python3
"""Compare public generated request failures before an entity data service exists."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('entry_lookup_5_0_6_cases.json.gz', 'Generated entry lookup'))
