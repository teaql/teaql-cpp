"""Observe the original repository extraction independently of audited saving."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("graph_repository_cases.json.gz", "Graph repository extraction"))
