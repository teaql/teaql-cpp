"""Compare original historical calls replayed against locked Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("audited_save_5_0_6_cases.json.gz", "audited_save 5.0.6"))
