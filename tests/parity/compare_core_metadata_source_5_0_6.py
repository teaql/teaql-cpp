#!/usr/bin/env python3
"""Compare native64 metadata boundaries with actual locked Rust observations."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('core_metadata_source_boundaries_5_0_6_cases.json.gz',
                          'Core metadata source boundaries 5.0.6'))
