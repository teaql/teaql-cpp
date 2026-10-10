"""Compare actual runtime graph model derives without safe-log projection."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("runtime_graph_model_cases.json.gz", "Runtime graph models"))
