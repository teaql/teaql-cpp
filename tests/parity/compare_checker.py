"""Compare checker and Context calls against original Rust public APIs."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('checker_cases.json', 'Checker/fix runtime'))
