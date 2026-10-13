"""Compare complete historical typed-query inputs replayed against locked Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("id_set_page_5_0_6_cases.json.gz", "id_set_page 5.0.6"))
