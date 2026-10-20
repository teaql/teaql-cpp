"""Compare one-shot diagnostics, ordered fallback journals and actual Context close."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_diagnostics_guard_5_0_6_cases.json.gz', 'SQL diagnostic guards'))
