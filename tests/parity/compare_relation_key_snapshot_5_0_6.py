#!/usr/bin/env python3
"""Compare key-snapshot attachment with actual locked private-helper outcomes."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('relation_key_snapshot_5_0_6_cases.json.gz',
                          'Relation aggregate key snapshot 5.0.6'))
