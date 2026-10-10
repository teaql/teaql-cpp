"""Compare original raw/safe audit event model diagnostics and derives."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("event_model_cases.json.gz", "Audit event models"))
