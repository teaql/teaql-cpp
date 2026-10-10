#!/usr/bin/env python3
"""Observe source core query/Expr public range factories and cache builders."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    def value(kind, data=None):
        return dict(kind=kind, value=data)
    pool = [value("null"), value("typed_null"), value("bool", True),
            value("i64", "-9223372036854775808"), value("u64", "18446744073709551615"),
            value("f64", "NaN"), value("f64", "-0.0"), value("decimal", "1.20"),
            value("text", "雪\n\0"), value("date", "2024-02-29"), value("timestamp", "42"),
            value("json", {"x": [1, None]}), value("list", [value("text", "nested")]),
            value("object", {"x": value("i64", "42")})]
    cases = []
    for function in range(14):
        for count in [0, 1, 2, 20, 21]:
            for element in pool:
                cases.append(dict(entity="雪Order", projections=[[], ["x", "x", "雪"], ["", "x"]],
                                  expires=["0", "1", "18446744073709551615"][count % 3],
                                  propagate=["0", "99", "18446744073709551615"][function % 3],
                                  function=function, values=[element] * count))
    for index, lower in enumerate(pool):
        for column, pattern in [("", ""), ("雪\0column", "%_\\雪\0"), ("key", "x" * 128)]:
            cases.append(dict(kind="full_expression", column=column, pattern=pattern,
                              lower=lower, upper=pool[(index + 5) % len(pool)], filter=index % 2 == 0))
    for index, payload in enumerate(pool):
        cases.append(dict(kind="query_builders", column=["", "field雪\0"][index % 2],
                          pattern=["", "%_\\雪\0", "x" * 128][index % 3], value=payload))
    for index, payload in enumerate(pool):
        cases.append(dict(kind="query_helpers", column=["", "field雪\0"][index % 2],
                          pattern=["", "%_\\雪\0", "x" * 128][index % 3], value=payload))
    for namespace in ["", " \t\n", "\u00a0", "\u3000", " ns雪 ", "\0"]:
        for ttl in ["0", "1"]:
            cases.append(dict(kind="validate_query", type="continuous", namespace=namespace,
                              ttl=ttl, maximum="0"))
            for maximum in ["0", "1"]:
                cases.append(dict(kind="validate_query", type="id_set", namespace=namespace,
                                  ttl=ttl, maximum=maximum))
    for maximum in ["0", "1", "18446744073709551615"]:
        cases.append(dict(kind="validate_query", type="hard_limit", namespace="", ttl="0", maximum=maximum))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"], note="Original core public query/Expr range factories and standalone cache builders; all Expr functions and Value variants.",
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, outputs)])
    Path(__file__).with_name("query_api_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} original core query API inputs")


if __name__ == "__main__":
    main()
