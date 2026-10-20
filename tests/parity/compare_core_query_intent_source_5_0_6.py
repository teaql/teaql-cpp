#!/usr/bin/env python3
"""Compare core purpose storage separately from runtime purpose trace creation."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('core_query_intent_source_5_0_6_cases.json.gz',
                          'Core/runtime query intent 5.0.6'))
