"""Compare actual Context schema initialization with all ordered provider phases."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('schema_dynamic_source_5_0_6_cases.json.gz',
                         'Context dynamic schema lifecycle'))
