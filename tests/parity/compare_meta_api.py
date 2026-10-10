#!/usr/bin/env python3
"""Compare complete descriptors, Debug flags, builder defaults and lookup order."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('meta_api_cases.json.gz', 'Metadata API'))
