#!/usr/bin/env python3
"""Compare the complete original DynamicFieldError value diagnostic protocol."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('dynamic_field_error_debug_5_0_6_cases.json.gz',
                          'Dynamic field error diagnostics 5.0.6'))
