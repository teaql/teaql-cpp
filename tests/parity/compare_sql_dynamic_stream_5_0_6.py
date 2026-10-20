"""Compare dynamic stream entry gates, metadata hooks and cursor cleanup."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('sql_dynamic_stream_5_0_6_cases.json.gz', 'SQL dynamic stream entry'))
