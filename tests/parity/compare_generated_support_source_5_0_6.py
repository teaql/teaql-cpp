"""Compare complete executable/private query state with the locked source."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('generated_support_source_5_0_6_cases.json.gz',
                         'Generated support private source'))
