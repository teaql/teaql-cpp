#!/usr/bin/env python3
"""Record complete SmartList states from unchanged Rust 5.0.5 public calls."""
import argparse
import gzip
import json
import random
import subprocess
from pathlib import Path

from compare_values import observe


def value(kind, data=None):
    return dict(kind=kind, value=data)


def facet(element, depth=0):
    return dict(
        data=[dict(names=["x", "x", "雪"], values=[element, value("text", "last"), value("null")])],
        loaded=depth % 2 == 0,
        total="18446744073709551615" if depth == 0 else None,
        aggregations={"count": value("u64", "9")},
        summary={"caption": element},
        facets={"child": facet(element, depth + 1)} if depth < 2 else {},
    )


def inputs():
    pool = [
        value("null"), value("typed_null"), value("bool", True), value("bool", False),
        value("i64", "-9223372036854775808"), value("i64", "7"),
        value("u64", "18446744073709551615"), value("f64", "NaN"),
        value("f64", "-0.0"), value("f64", "inf"), value("decimal", "1.20"),
        value("date", "2024-02-29"), value("timestamp", "-9223372036854775808"),
        value("text", ""), value("text", "雪😀a\0b"), value("text", "v" * 2048),
        value("json", {"nested": ["long" * 16, 2, None]}),
        value("list", [value("null"), value("text", "nested")]),
        value("object", {"k": value("list", [value("typed_null"), value("text", "nested")])}),
    ]
    cases = []
    for construct in ["default", "new"]:
        cases.append(dict(construct=construct, data=[], ops=[]))
        for element in pool:
            steps = [
                dict(op="push", value=element),
                dict(op="index", index=0), dict(op="index_mut", index=0, value=element),
                dict(op="get_mut", index=0, value=element),
                dict(op="get_mut", index=4294967295, value=element),
                dict(op="total", value="18446744073709551615"),
                dict(op="aggregation", key="same", value=element),
                dict(op="summary", key="same", value=element),
                dict(op="aggregation_lookup", key="same"),
                dict(op="summary_lookup", key="absent"),
                dict(op="facet", key="same", value=facet(element)),
                dict(op="facet_lookup", key="same"),
                dict(op="with_facet", key="same", value=facet(value("text", "replacement"))),
                dict(op="facets_mut", key="second", value=facet(element)),
                dict(op="facet_mut", key="same", value=False),
                dict(op="extend", values=[element, value("i64", "9")]),
                dict(op="self_extend"),
                dict(op="set", index=1, value=value("text", "changed")),
                dict(op="set", index=4294967295, value=element),
                dict(op="get", index=1), dict(op="get", index=4294967295),
                dict(op="to_list"), dict(op="to_set"), dict(op="identity"), dict(op="group"),
                dict(op="merge", values=[element, element, value("text", "new")]),
                dict(op="self_merge"), dict(op="map"),
                dict(op="iter_mut", value=element), dict(op="slice_mut", value=element),
                dict(op="retain", value=element),
                dict(op="remove_facet", key="same"), dict(op="remove_facet", key="absent"),
                dict(op="take_facets"), dict(op="take_facets"),
                dict(op="loaded", value=False), dict(op="collect"),
            ]
            cases.append(dict(construct=construct, data=[] if construct == "default" else [element] * 2, ops=steps))
    rng = random.Random(505)
    operations = ["push", "extend", "self_extend", "set", "get", "get_mut", "loaded", "total",
                  "aggregation", "summary", "facet", "with_facet", "facets_mut", "facet_mut",
                  "facet_lookup", "aggregation_lookup", "summary_lookup", "remove_facet", "take_facets",
                  "collect", "map", "iter_mut", "slice_mut", "retain", "to_list", "to_set", "identity", "group", "merge", "self_merge"]
    for _ in range(256):
        construct = rng.choice(["default", "new"])
        steps = []
        for _ in range(20):
            name = rng.choice(operations)
            op = dict(op=name, key=rng.choice(["", "same", "second", "雪", "absent"]))
            if name in ["push", "set", "get_mut", "aggregation", "summary", "iter_mut", "slice_mut", "retain"]:
                op["value"] = rng.choice(pool)
            if name in ["get", "set", "get_mut"]:
                op["index"] = rng.choice([0, 1, 7, 4294967295])
            if name in ["extend", "merge"]:
                op["values"] = rng.choices(pool, k=rng.randrange(5))
            if name in ["loaded", "facet_mut"]:
                op["value"] = rng.choice([False, True])
            if name == "total":
                op["value"] = rng.choice(["0", "1", "9", "18446744073709551615"])
            if name in ["facet", "with_facet", "facets_mut"]:
                op["value"] = facet(rng.choice(pool))
            steps.append(op)
        cases.append(dict(construct=construct, data=rng.choices(pool, k=rng.randrange(6)), ops=steps))
    for loaded in [False, True]:
        empty_meta = facet(value("null"))
        empty_meta["loaded"] = loaded
        cases.append(dict(entities=[], metadata=empty_meta))
        for identifier in pool:
            for new, deleted, dirty in [(False, False, None), (True, False, None),
                                       (False, True, None), (False, False, []),
                                       (False, False, ["payload"]), (True, True, [])]:
                items = []
                for index in range(3):
                    items.append(dict(fields={"id": identifier, "payload": pool[(index + 12) % len(pool)],
                                               "order": value("u64", str(index))},
                                      version=["-9223372036854775808", "0", "9223372036854775807"][index],
                                      new=new if index == 1 else False, deleted=deleted if index == 1 else False,
                                      dirty=dirty if index == 1 else None))
                meta = facet(identifier)
                meta["loaded"] = loaded
                cases.append(dict(entities=items, metadata=meta))
    cases.extend(dict(iterator_trace=fail) for fail in [False, True])
    for method in ['extend', 'collect']:
        cases.extend(dict(iterator_trace=fail, method=method) for fail in [False, True])
    cases.extend(dict(iterator_trace=True, fail_stage='key', fail_at=item)
                 for item in [1, 2, 12, 3, 14])
    for method in ['retain', 'map', 'to_list', 'to_set', 'identity', 'group']:
        cases.extend(dict(callback_trace=method, fail_at=item) for item in range(6))
    cases.extend(dict(equality=field) for field in [
        'same', 'data', 'data_order', 'total_count', 'aggregation', 'summary', 'facets',
        'is_loaded', 'facet_count', 'facet_data', 'facet_loaded', 'facet_summary',
        'nan_data', 'nan_aggregation', 'nan_summary', 'nan_facet'])
    cases.extend(dict(drop_trace=fail_at) for fail_at in range(6))
    cases.extend(dict(boolean=seed) for seed in range(128))
    cases.extend(dict(drop_map=fail_at) for fail_at in range(6))
    float_bits = [0, 1, 0x007fffff, 0x00800000, 0x3dcccccd, 0x3f800000,
                  0x3f800001, 0x4b800000, 0x5a0e1bca, 0x7f7fffff,
                  0x7f800000, 0x7fc00000, 0x7fffffff]
    float_bits.extend(bits | 0x80000000 for bits in list(float_bits))
    bits_rng = random.Random(50532)
    float_bits.extend(bits_rng.getrandbits(32) for _ in range(512))
    cases.extend(dict(float32=bits) for bits in float_bits)
    cases.extend(dict(drop_entity=fail_at, projection=projection, loaded=loaded)
                 for fail_at in range(6) for projection in ['value', 'values']
                 for loaded in [False, True])
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
    corpus = dict(baseline=lock["commit"], note="Unchanged SmartList public calls and complete recursive states; all Value variants; seed 505.",
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, outputs)])
    data = (json.dumps(corpus, indent=2) + "\n").encode("utf-8")
    packed = gzip.compress(data, mtime=0)
    Path(__file__).with_name("smart_list_api_cases.json.gz").write_bytes(packed)
    print(f"Recorded {len(cases)} original SmartList sequences: {len(data)} bytes -> {len(packed)} gzip bytes")


if __name__ == "__main__":
    main()
