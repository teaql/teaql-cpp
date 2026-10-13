#!/usr/bin/env python3
"""Compare freshly replayed source time errors without relabeling old evidence."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('system_time_error_scope_5_0_6_cases.json.gz',
                         'System-time error 5.0.6'))
