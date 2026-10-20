"""Compare unchanged original public entity-reference codec and Context calls."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("entity_reference_baseline_5_0_6_cases.json.gz", "Entity reference"))
