"""Record original recursive relation plan diagnostics and value semantics."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def base():
    return dict(parent="Order", name="items", path="items.parts", target="Item", local="id",
                foreign="order_id", many=True, query=None, children=[])


def inputs():
    cases = []
    values = [dict(kind="text", value="用户🧊"), dict(kind="u64", value="18446744073709551615"),
              dict(kind="f64", value="NaN"), dict(kind="typed_null")]
    queries = [None] + [dict(entity="Item", value=value, comment="reason\0🧊", limit="17", offset="1", threshold=threshold)
                       for value, threshold in itertools.product(values, [0, 7])]
    for query, depth, changed in itertools.product(queries, [0, 2],
            ["parent", "name", "path", "target", "local", "foreign", "many", "query", "children"]):
        value = base()
        value["query"] = query
        if depth:
            child = base()
            child.update(parent="Item", name="parts", target="Part", query=query)
            grandchild = base()
            grandchild.update(parent="Part", name="owner", target="Order", many=False)
            child["children"] = [grandchild]
            value["children"] = [child]
        value["changed"] = changed
        cases.append(value)
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Repeated original relation model observations disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original public RelationLoadPlan and full SelectQuery constructors/methods, recursive Clone/PartialEq/Debug in nine modes, every field difference including optional query/children. Native-width probe thresholds stay in the shared range; Values include full-width numbers/NaNs. No execution query or graph algorithm is reimplemented by the model oracle.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name("relation_model_cases.json.gz")), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, separators=(",", ":"), ensure_ascii=True).encode())
    print(f"Recorded {len(cases)} twice-agreeing original relation plan models")


if __name__ == "__main__":
    main()
