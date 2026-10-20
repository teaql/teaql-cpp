#!/usr/bin/env python3
"""Compare actual Rust SmartList ID keys and JSON text, including float ties."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("format_5_0_6_cases.json.gz", "ID/JSON text"))
