#!/usr/bin/env python3
"""Compare pagination convenience execution against actual original Rust."""
from compare_values import main
if __name__=="__main__":
    raise SystemExit(main("pagination_convenience_5_0_6_cases.json.gz","Pagination convenience 5.0.6"))
