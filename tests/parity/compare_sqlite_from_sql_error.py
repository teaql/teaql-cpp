#!/usr/bin/env python3
"""Compare actual FromSqlError payloads and portable conversion responsibilities."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("sqlite_from_sql_error_5_0_6_cases.json.gz", "SQLite column-conversion errors"))
