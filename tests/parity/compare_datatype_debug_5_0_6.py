#!/usr/bin/env python3
"""Compare standalone DataType diagnostics against separately replayed 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("datatype_debug_5_0_6_cases.json.gz", "DataType Debug 5.0.6"))
