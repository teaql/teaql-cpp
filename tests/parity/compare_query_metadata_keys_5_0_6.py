#!/usr/bin/env python3
"""Compare readonly schema and intent participation in native64 query keys."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('query_metadata_keys_5_0_6_cases.json.gz', 'Query metadata keys'))
