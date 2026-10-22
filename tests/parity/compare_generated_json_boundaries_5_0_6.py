#!/usr/bin/env python3
"""Compare input boundaries independently of unresolved generated payload metadata."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('generated_json_boundaries_5_0_6_cases.json.gz', 'Generated JSON input boundaries'))
