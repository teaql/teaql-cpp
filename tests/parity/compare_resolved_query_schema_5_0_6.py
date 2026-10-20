"""Compare actual readonly query-result schema and cache boundaries."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('resolved_query_schema_5_0_6_cases.json.gz', 'Resolved query schema 5.0.6'))
