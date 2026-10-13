"""Compare complete historical typed-query inputs replayed against locked Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("entity_query_5_0_6_cases.json.gz", "entity_query 5.0.6"))
