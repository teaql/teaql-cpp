"""Compare complete historical inputs replayed against locked Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("context_model_5_0_6_cases.json.gz", "context_model 5.0.6"))
