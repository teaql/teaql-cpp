"""Compare readonly stream results and cursor consumption against locked source."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('resolved_stream_schema_5_0_6_cases.json.gz', 'Resolved stream schema 5.0.6'))
