#!/usr/bin/env python3
"""Compare selection/corruption boundaries against unchanged original observations."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('sqlite_dynamic_storage_boundaries_5_0_6_cases.json.gz',
                          'SQLite dynamic storage boundaries 5.0.6'))
