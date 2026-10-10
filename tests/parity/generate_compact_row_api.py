#!/usr/bin/env python3
"""Generate original-call observations, retaining duplicate layouts; seed 505."""
import argparse
import json
import gzip
import random
import subprocess
from pathlib import Path
from compare_values import observe


def value(kind, data=None):
    return dict(kind=kind, value=data)


def inputs():
    pool = [value("null"), value("typed_null"), value("i64", "7"), value("u64", "18446744073709551615"),
            value("text", "雪😀a\0b"), value("f64", "NaN"), value("decimal", "1.20"),
            value("json", {"nested": ["long" * 64]}), value("text", "v" * 4096),
            value("object", {"k": value("list", [value("text", "nested"), value("typed_null")])})]
    cases = []
    operations = [dict(op="save"), dict(op="get", name="x"), dict(op="set", name="x", value=value("text", "changed")),
                  dict(op="insert", name="new", value=value("i64", "9")), dict(op="save"),
                  dict(op="remove", name="x"), dict(op="self_extend"), dict(op="clone"),
                  dict(op="extend", names=["x", "x", "new"], values=[value("i64", "1"), value("i64", "2"), value("null")]),
                  dict(op="save"), dict(op="remove", name="absent"), dict(op="consume")]
    for names in [[], ["x"], ["x", "x"], ["z", "x", "x", "a"], ["", "雪", "x"]]:
        for element in pool:
            for from_map in [False, True]:
                cases.append(dict(names=names, values=[element] * len(names), from_map=from_map, ops=operations))
    rng = random.Random(505)
    names_pool = ["x", "x", "z", "", "雪", "long" * 64]
    for _ in range(256):
        names = rng.choices(names_pool, k=rng.randrange(0, 9))
        ops = [dict(op="save")]
        for _ in range(16):
            op = rng.choice(["insert", "remove", "get", "set", "clone", "save", "self_extend", "extend", "consume"])
            entry = dict(op=op, name=rng.choice(names_pool))
            if op in ["insert", "set"]:
                entry["value"] = rng.choice(pool)
            if op == "extend":
                extra = rng.choices(names_pool, k=rng.randrange(0, 5))
                entry.update(names=extra, values=rng.choices(pool, k=len(extra)))
            ops.append(entry)
        cases.append(dict(names=names, values=rng.choices(pool, k=len(names)), from_map=rng.choice([False, True]), ops=ops))
    json_pool = pool + [value("bool", True), value("bool", False), value("date", "2032-02-29"),
                        value("timestamp", "-9223372036854775808"), value("list", [value("null"), value("text", "雪")])]
    for payload in json_pool:
        for names in [[], ["x"], ["x", "x", "z", "雪\0"]]:
            entries = [payload] * len(names)
            if len(entries) > 1:
                entries[0] = value("null")
            cases.append(dict(kind="json_projection", names=names, values=entries))
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
    corpus = dict(baseline=lock["commit"], note="Original CompactRow public methods, Arc layout identity/snapshots and duplicate columns; seed 505.",
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, outputs)])
    data = (json.dumps(corpus, indent=2) + "\n").encode("utf-8")
    Path(__file__).with_name("compact_row_api_cases.json.gz").write_bytes(gzip.compress(data, mtime=0))
    print(f"Recorded {len(cases)} original CompactRow observations")


if __name__ == "__main__":
    main()
