"""Record original purpose wrapper diagnostics without copying its implementation."""
import argparse
import copy
import gzip
import json
from pathlib import Path
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    source = json.loads(Path(__file__).with_name("generated_support_cases.json").read_text())
    root = Path(__file__).resolve().parents[2]
    lock = json.loads((root / "upstream.lock.json").read_text())
    if source["baseline"] != lock["commit"]:
        raise RuntimeError("Generated support inputs belong to another baseline")
    inputs = []
    for case in source["cases"]:
        if case["input"]["op"] == "purpose":
            value = copy.deepcopy(case["input"])
            value["protocol"] = True
            inputs.append(value)
    base = copy.deepcopy(inputs[0])
    purposes = ["", " ", "\t\n", "\u0085\u2003", "\u200b", "reason", "  reason  ",
                'why "quoted"\\\n', "语义测试", "emoji 🧊", "token=secret", "r" * 1024]
    queries = [{"entity": "Simple"}, base["outer"],
               {"entity": "🧊", "projection": ["long" * 64],
                "trace": [{"kind": 7, "entity": "🧊", "comment": "previous", "id": 0}]}]
    for query in queries:
        for purpose in purposes:
            value = copy.deepcopy(base)
            value["outer"] = copy.deepcopy(query)
            value["purpose"] = purpose
            inputs.append(value)
    first = observe(args.rust, inputs)
    if not same(first, observe(args.rust, inputs)):
        raise RuntimeError("Repeated original observations disagree")
    corpus = {"baseline": source["baseline"],
              "note": "Original Rust PurposedSelectQuery constructors, derived Debug/Clone, as_query and into_query; two original executions agree. General PurposedQuery has no derived Debug contract.",
              "cases": [{"input": value, "expected": output}
                        for value, output in zip(inputs, first)]}
    path = Path(__file__).with_name("purposed_query_debug_cases.json.gz")
    with gzip.GzipFile(filename=str(path), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(",", ":")).encode())
    print(f"Recorded {len(inputs)} twice-agreeing purpose wrapper observations")


if __name__ == "__main__":
    main()
