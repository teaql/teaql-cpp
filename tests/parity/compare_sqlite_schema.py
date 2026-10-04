#!/usr/bin/env python3
"""Compare production physical schema against Rust 5.0.5."""
from compare_values import main
if __name__ == '__main__':
    raise SystemExit(main('sqlite_schema_cases.json', 'Production SQLite schema'))
