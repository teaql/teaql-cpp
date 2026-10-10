"""Compare actual non-Clone original transaction output and cleanup semantics."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("transaction_move_only_cases.json", "Move-only transaction"))
