#!/usr/bin/env python3
"""Decode and read back actual generated typed JSON scalar and graph projections."""
import argparse
import copy
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / "rust_field_layout/src/bin/generated_json_hydration_5_0_6.rs"
TARGET = Path(__file__).with_name("generated_json_column_alias_5_0_6_cases.json.gz")


def inputs():
    entries = []
    common = [None, True, 1, [], {}, {"id": 1}, {"id": 1, "version": 3},
              {"id": 1, "version": -3}, {"id": 1, "unknown": "PRIVATE_LITERAL"},
              {"id": 1, "_label": "readonly"}, {"id": 1, "_label": None},
              {"id": 1, "_label": {"items": [1, None]}},
              {"id": 1, "#private": "PRIVATE_LITERAL"}, {"_dirty_fields": ["name"]}]
    modeled = {
        "Platform": [{"id": 1, "name": "tenant", "person_list": []},
                     {"id": 1, "person_list": [{"id": 2, "name": "Ada", "platform": {"id": 1}}]}],
        "Person": [{"id": 2, "name": "Ada", "email": "a@example.test"},
                   {"id": 2, "name": None}, {"id": 2, "platform": {"id": 1}},
                   {"id": 2, "platform_id": None, "platform": None},
                   {"id": 2, "profile": None}, {"id": 2, "profile": {"id": 3, "note": None}},
                   {"id": 2, "orders": []},
                   {"id": 2, "orders": [{"id": 3, "code": "first"}, {"id": 3, "priority": 4}]},
                   {"id": 2, "orders": [{"id": 3, "code": "first"}, {"id": 3, "code": "second"}]}],
        "Order": [{"id": 3, "code": "order", "amount": "12.50", "priority": 4},
                  {"id": 3, "customer": {"id": 2, "name": "Ada"}},
                  {"id": 3, "customer_id": 9, "customer": {"id": 2}},
                  {"id": 3, "customer": None}, {"id": 3, "lines": []},
                  {"id": 3, "lines": [{"id": 4, "quantity": 2, "product": {"id": 5, "sku": "p"}}]},
                  {"id": 3, "lines": [{"id": 4, "order_id": 3, "order": {"id": 3}}]},
                  {"id": 3, "customer": {"id": 2, "orders": [{"id": 3}]}},
                  {"id": 3, "lines": [{"id": 4, "order_id": 9}]}],
        "Line": [{"id": 4, "quantity": 2}, {"id": 4, "order_id": 3},
                 {"id": 4, "order": {"id": 3, "code": "order"}},
                 {"id": 4, "order": None}, {"id": 4, "product": {"id": 5, "price": "1.20"}}],
        "Product": [{"id": 5, "sku": "p", "price": "1.20"},
                    {"id": 5, "line_list": []}, {"id": 5, "line_list": [{"id": 4, "product_id": 5}]}],
        "Profile": [{"id": 6, "note": None}, {"id": 6, "note": "text", "person": {"id": 2}},
                    {"id": 6, "person": None}],
        "Company": [{"id": 7, "name": "company", "platform": {"id": 1}}],
    }
    for entity, specific in modeled.items():
        for payload in common + specific:
            entries.append(dict(entity=entity, array=False, value=payload))
        for array in ([], [{"id": 1}, {"id": 2}], [{"id": 1, "_note": "a"}, {"id": 1, "_note": "b"}],
                      [specific[0], specific[-1]], [specific[0], True]):
            entries.append(dict(entity=entity, array=True, value=array))
    # Preserve the existing 165 observations, then append source column alias controls.
    for key in ('order', 'order_id', '"order"'):
        for value in (3, None, {'id': 3}):
            entries.append(dict(entity='Line', array=False, value={key: value}))
    for entity, keys in [('Person', ('platform', 'platform_id')), ('Profile', ('person', 'person_id')),
                         ('Order', ('customer', 'customer_id', 'seller', 'seller_id')),
                         ('Line', ('product', 'product_id'))]:
        for key in keys:
            entries.append(dict(entity=entity, array=False, value={key: 3}))
    entries.extend([
        dict(entity='Line', array=False, value={'order_id': 3, '"order"': 3}),
        dict(entity='Line', array=False, value={'order_id': 3, '"order"': 4}),
        dict(entity='Order', array=False, value={'id': 3, 'lines': [{'id': 4, '"order"': 3}]}),
        dict(entity='Order', array=False, value={'id': 3, 'lines': [{'id': 4, '"order"': 9}]}),
        dict(entity='Person', array=False, value={'id': 2, 'orders': [{'id': 3, 'lines': [{'id': 4, '"order"': 3}]}]}),
    ])
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    stimuli = inputs()
    outputs = observe(args.rust, stimuli)
    corpus = dict(baseline=lock["commit"], version=lock["version"], driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Original generated domain module, Context JSON decoding and generated Entity JSON readback/load/dirty status. No output normalization, SQL or persistence authority qualification.",
                  cases=[dict(input=i, expected=o) for i, o in zip(stimuli, outputs)])
    if args.check:
        with gzip.open(TARGET, "rt", encoding="utf-8") as f:
            assert same(corpus, json.load(f)), "Corpus differs from actual source"
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    if args.cpp:
        actual = observe(args.cpp, stimuli)
        gaps = [dict(index=n, input=i, rust=a, cpp=b)
                for n, (i, a, b) in enumerate(zip(stimuli, outputs, actual)) if not same(a, b)]
        if gaps:
            path = ROOT / "docs/parity/generated_json_hydration_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} generated typed JSON observations differ")
    print(f"Reproduced {len(stimuli)} actual generated typed JSON observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
