#!/usr/bin/env python3
"""Compare direct IN compiler defaults and their partial failure state."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sql_dialect_in_cases.json", "Direct SQL IN defaults"))
