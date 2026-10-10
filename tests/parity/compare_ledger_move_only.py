"""Compare actual original non-Clone entity save behavior."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("ledger_move_only_cases.json.gz", "Move-only ledger save"))
