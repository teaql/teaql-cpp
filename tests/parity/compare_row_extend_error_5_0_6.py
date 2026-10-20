#!/usr/bin/env python3
"""Compare unchecked row-extension failures with the original panic boundary."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('row_extend_error_5_0_6_cases.json.gz', 'Row extension error 5.0.6'))
