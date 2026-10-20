#!/usr/bin/env python3
"""Compare production physical schema against locked Rust 5.0.6."""
from compare_values import main
if __name__ == '__main__':
    raise SystemExit(main('sqlite_schema_5_0_6_cases.json.gz', 'Production SQLite schema'))
