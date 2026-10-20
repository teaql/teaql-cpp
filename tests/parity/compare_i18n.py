"""Compare language/catalog/Context results with original Rust public calls."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('i18n_baseline_5_0_6_cases.json.gz', 'Language and i18n'))
