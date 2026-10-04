#!/usr/bin/env python3
"""Compare persistent business-ID allocation against the pinned provider."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sqlite_business_id_cases.json", "Production SQLite business ID"))
