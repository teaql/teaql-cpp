"""Compare full dynamic state/change-set diagnostics against the fixed source."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('entity_runtime_dynamic_debug_5_0_6_cases.json.gz', 'Dynamic entity-state diagnostics'))
