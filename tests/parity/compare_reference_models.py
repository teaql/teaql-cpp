"""Compare original reference models and redacted key diagnostics."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("reference_model_cases.json.gz", "Reference models"))
