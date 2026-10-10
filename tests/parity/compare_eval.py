#!/usr/bin/env python3
"""Compare published evaluation and load-state contracts with pinned Rust."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('eval_cases.json.gz', 'Evaluation and load states'))
