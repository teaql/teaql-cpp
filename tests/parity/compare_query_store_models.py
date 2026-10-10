"""Compare original pagination/cache public model diagnostics and derives."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("query_store_model_cases.json.gz", "Query store models"))
