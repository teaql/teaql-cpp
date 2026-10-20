#!/usr/bin/env python3
"""Compare actual SQLite public facet diagnostics and unrelated follow-up."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('public_facet_logs_5_0_6_cases.json.gz', 'Public facet SQL privacy'))
