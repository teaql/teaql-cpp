#!/usr/bin/env python3
"""Observe original model traits, including every command field."""
import argparse
import copy
import json
import subprocess
from pathlib import Path
from compare_values import observe


def val(kind, value=None):
    return dict(kind=kind, value=value)


def node(kind=3, default=False):
    return dict(trace_kind=kind, default=default, entity="Order", id="7", comment="audit")


def inputs():
    values = [val("null"), val("typed_null"), val("bool", True), val("i64", "1"),
              val("u64", "1"), val("i64", "-9223372036854775808"), val("u64", "18446744073709551615"),
              val("f64", "0.0"), val("f64", "-0.0"), val("f64", "NaN"), val("f64", "inf"),
              val("decimal", "1.00"), val("decimal", "1"), val("text", "雪😀a\0b"),
              val("json", {"n": 1, "payload": [None, "x"]}), val("date", "2032-02-29"),
              val("timestamp", "-9223372036854775808"), val("list", [val("typed_null"), val("text", "long" * 64)]),
              val("object", {"nested": val("text", "value"), "x": val("null")}),
              val("list", [val("f64", "NaN")])]
    records = [{}] + [{"z": value, "a": val("text", "prefix")} for value in values]
    cases = []
    for kind in ["mutation", "generated", "snapshot"]:
        for left in records:
            for right in records:
                cases.append(dict(kind="wrapper", type=kind, left=left, right=right))
    for kind in range(9):
        left = node(kind)
        alternatives = [left, node((kind + 1) % 9), dict(left, entity=""), dict(left, comment=""),
                        dict(left, id=None), dict(left, id="0"), dict(left, id="18446744073709551615"),
                        dict(left, default=True), dict(left, entity="雪😀", comment="a\0b")]
        for right in alternatives:
            cases.append(dict(kind="trace", left=left, right=right))
    for kind in range(9):
        cases.append(dict(kind="trace", left=node(kind, True), right=node(3)))
    for kind in ["insert", "update", "delete", "recover", "batch_insert", "batch_update"]:
        base = dict(entity="Order", id=val("u64", "7"), version="1", soft=True,
                    values={"name": val("text", "Riverside")}, old={"name": val("text", "before")}, trace=[node()])
        if kind == "recover":
            base["version"] = "-3"
        if kind.startswith("batch"):
            base.update(values=[base["values"], {"name": val("text", "Lakeside")}], trace=[[node()], []])
        if kind == "batch_update":
            base.update(ids=[val("u64", "7"), val("u64", "8")], versions=[None, "1"],
                        fields=["name", "status", "name"], old=[None, {}, {"name": val("text", "before")}])
        fields = {"insert": ["entity", "values", "trace"],
                  "update": ["entity", "id", "version", "values", "trace", "old"],
                  "delete": ["entity", "id", "version", "soft", "trace"],
                  "recover": ["entity", "id", "version", "trace"],
                  "batch_insert": ["entity", "values", "trace"],
                  "batch_update": ["entity", "ids", "versions", "values", "fields", "trace", "old"]}[kind]
        cases.append(dict(kind="command", type=kind, left=base, right=base))
        for field in fields:
            alternatives = {
                "entity": ["", "Invoice", "雪Order"], "id": values,
                "version": ["0", "-1", "9223372036854775807"] + ([] if kind == "recover" else [None]),
                "soft": [False], "ids": [[], [val("u64", "8"), val("u64", "7")], [val("f64", "NaN")]],
                "versions": [[], ["0", "1"], ["1", None]], "fields": [[], ["status", "name", "name"], [""]],
                "trace": ([[], [[], [node()]], [[dict(node(), comment="changed")], []]] if kind.startswith("batch")
                          else [[], [dict(node(), comment="changed")], [node(8)], [node(), node()]]),
                "old": ([[], [None], [{}], [{"v": val("f64", "NaN")}]] if kind == "batch_update"
                        else [None, {}, {"v": val("f64", "NaN")}]),
                "values": []
            }
            if field == "values":
                alternatives[field] = [[], list(reversed(base["values"]))] + [[{"v": v}] for v in values] if kind.startswith("batch") else records
            for replacement in alternatives[field]:
                right = copy.deepcopy(base)
                right[field] = replacement
                cases.append(dict(kind="command", type=kind, left=base, right=right))
        # Self and clone comparisons also exercise NaN in nested command fields.
        for field in [f for f in fields if f in ["id", "values", "old"]]:
            nan = copy.deepcopy(base)
            if field == "id":
                nan[field] = val("f64", "NaN")
            elif kind.startswith("batch"):
                nan[field] = [{"nan": val("f64", "NaN")}]
            else:
                nan[field] = {"nan": val("f64", "NaN")}
            cases.append(dict(kind="command", type=kind, left=nan, right=nan))
    for payload in values:
        puts = [dict(op="value", field="repeat", value=val("null")),
                dict(op="value", field="repeat", value=payload),
                dict(op="value", field="雪\0", value=payload),
                dict(op="value", field="repeat", value=val("text", "last"))]
        cases.append(dict(kind="builders", type="insert", entity="Order雪\0", steps=puts))
        steps = [dict(op="version", version="-9223372036854775808"),
                 *puts[:2], dict(op="version", version="9223372036854775807"),
                 *puts[2:], dict(op="version", version="0")]
        cases.append(dict(kind="builders", type="update", entity="Order雪\0", id=payload, steps=steps))
    for key in [val("null"), val("u64", "18446744073709551615"), val("text", "雪\0")]:
        cases.append(dict(kind="builders", type="delete", entity="Order雪\0", id=key,
                          steps=[dict(op="version", version="-9223372036854775808"),
                                 dict(op="hard_delete"), dict(op="hard_delete"),
                                 dict(op="version", version="9223372036854775807"),
                                 dict(op="version", version="0")]))
    cases.extend(dict(kind="map_methods", type=kind) for kind in ["mutation", "generated", "snapshot"])
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
    corpus = dict(baseline=lock["commit"], note="Original public PartialEq, Record conversions/iteration and trace constructors.",
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, outputs)])
    Path(__file__).with_name("mutation_model_api_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} original mutation/trace model API observations")


if __name__ == "__main__":
    main()
