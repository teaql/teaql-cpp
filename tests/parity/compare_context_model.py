#!/usr/bin/env python3
"""Compare pinned Context model derives and constructor failure precedence."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('context_model_cases.json.gz', 'Context model'))
