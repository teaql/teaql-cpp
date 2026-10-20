#!/usr/bin/env python3
"""Compare the original readonly query schema builder and sharing behavior."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('query_schema_builder_5_0_6_cases.json.gz',
                          'Query schema builder 5.0.6'))
