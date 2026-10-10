#!/usr/bin/env python3
"""Compare original SafeExpression/TeaqlEmpty methods and callback traces."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("safe_expression_api_cases.json", "SafeExpression API"))
