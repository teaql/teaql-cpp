"""Compare original runtime log model diagnostics and derives."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("log_model_cases.json.gz", "Runtime log models"))
