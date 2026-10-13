#!/usr/bin/env python3
"""Compare SafeExpression callback traces against separately replayed 5.0.6 calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("safe_expression_api_5_0_6_cases.json.gz", "SafeExpression API 5.0.6"))
