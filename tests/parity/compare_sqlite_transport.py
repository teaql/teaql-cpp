#!/usr/bin/env python3
"""Compare the production SQLite transport with the pinned Rust provider."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sqlite_transport_cases.json", "Production SQLite transport"))
