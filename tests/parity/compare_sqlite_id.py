#!/usr/bin/env python3
"""Compare persistent SQLite ID-space allocation with the pinned provider."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sqlite_id_cases.json", "Production SQLite ID-space"))
