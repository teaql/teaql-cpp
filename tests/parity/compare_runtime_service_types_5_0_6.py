#!/usr/bin/env python3
"""Compare shared runtime service types with actual private-source calls."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('runtime_service_types_5_0_6_cases.json.gz',
                          'Runtime service shared types 5.0.6'))
