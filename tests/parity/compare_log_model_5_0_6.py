"""Compare complete historical inputs replayed against locked Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("log_model_5_0_6_cases.json.gz", "log_model 5.0.6"))
