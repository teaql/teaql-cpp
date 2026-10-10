#!/usr/bin/env python3
"""Record original derived model Debug and inherited scalar formatting flags."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    def value(kind, data=None):
        return dict(kind=kind, value=data)
    pool = [value("null"), value("typed_null"), value("bool", True), value("bool", False),
            value("i64", "-9223372036854775808"), value("u64", "18446744073709551615"),
            value("f64", "NaN"), value("f64", "inf"), value("f64", "-inf"),
            value("f64", "-0.0"), value("f64", "1.2345"), value("f64", "1e-20"),
            value("f64", "1.7976931348623157e308"), value("decimal", "-0.00120"),
            value("decimal", "123456789.20"), value("text", "雪\n\0\t😀"),
            value("date", "2024-02-29"), value("timestamp", "-9223372036854775808"),
            value("json", {"nested": [1, 1.2345, False, None, "雪\n"]}),
            value("list", [value("bool", True), value("i64", "-42")]),
            value("object", {"k": value("list", [value("text", "nested"), value("timestamp", "42")])})]
    cases = []
    for element in pool:
        for loaded in [False, True]:
            for total in [None, "0", "42", "18446744073709551615"]:
                cases.append(dict(row=dict(names=["x", "x", "雪\n"], values=[element]*3),
                                  values=[element, value("null")], loaded=loaded, total=total,
                                  summary={"same": element}, aggregations={"count": value("u64", "42")}))
    cases.append(dict(row=dict(names=[], values=[]), values=[], loaded=False, total=None, summary={}, aggregations={}))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    results = observe(args.rust, cases)
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"], note="Unchanged source derived Debug for rows/lists, nine formatter flag combinations and all Value variants.",
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, results)])
    Path(__file__).with_name("model_debug_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} source model Debug cases")


if __name__ == "__main__":
    main()
