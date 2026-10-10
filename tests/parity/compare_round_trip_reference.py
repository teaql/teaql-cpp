"""Compare unchanged public RoundTrip models, providers, runtime and Context."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("round_trip_reference_cases.json", "Round-trip reference"))
