#!/usr/bin/env python3
"""Compare source generated membership observations with the current C++ runtime."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('generated_matching_any_5_0_6_cases.json.gz', 'Generated matching any'))
