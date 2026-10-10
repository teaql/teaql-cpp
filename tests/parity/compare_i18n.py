"""Compare language/catalog/Context results with original Rust public calls."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('i18n_cases.json', 'Language and i18n'))
