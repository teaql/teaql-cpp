#!/usr/bin/env python3
"""Compare default repeated SQL transport and executor errors with pinned Rust."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("sql_transport_5_0_6_cases.json.gz", "SQL transport defaults/errors"))
