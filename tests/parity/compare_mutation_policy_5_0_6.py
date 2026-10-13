"""Compare original historical calls replayed against locked Rust 5.0.6."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("mutation_policy_5_0_6_cases.json.gz", "mutation_policy 5.0.6"))
