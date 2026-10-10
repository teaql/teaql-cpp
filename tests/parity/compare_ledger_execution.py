"""Compare source ledger reads, generated IDs, mutation requests and events."""
from compare_values import main
if __name__ == '__main__':
    raise SystemExit(main('ledger_execution_cases.json', 'Ledger execution'))
