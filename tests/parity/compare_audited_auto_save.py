"""Compare original plain and automatic-ledger audited entry points."""
from compare_values import main
if __name__ == '__main__':
    raise SystemExit(main('audited_auto_save_cases.json', 'Automatic audited save'))
