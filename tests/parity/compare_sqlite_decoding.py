#!/usr/bin/env python3
"""Compare column decoding with the pinned provider's public fetch."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sqlite_decoding_cases.json", "SQLite row decoding"))
