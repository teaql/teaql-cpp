"""Compare original checker model diagnostics, derives and wire JSON."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("check_model_cases.json.gz", "Checker models"))
