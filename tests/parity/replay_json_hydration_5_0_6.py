#!/usr/bin/env python3
"""Observe original Context typed JSON scalar and recursive graph hydration."""
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
DRIVER = Path(__file__).parent / "rust_private_save_5_0_6/src/bin/json_hydration_5_0_6.rs"
TARGET = Path(__file__).with_name("json_hydration_5_0_6_cases.json.gz")


def inputs():
    values = [None, True, 1, "text", [], {}, {"id": 1},
              {"id": 1, "stored_label": "alias"}, {"id": 1, "label": None},
              {"label": "a", "stored_label": "b"}, {"_note": {"x": [1, None]}},
              {"_note": None}, {"#private": "secret"}, {"_": "secret"},
              {"unknown": "PRIVATE_LITERAL"}, {"nonnull": None}, {"fail": True}]
    for key in ("_comment", "_dirty_fields", "_original_values", "_is_new", "_is_deleted",
                "__load_state", "__teaql_runtime_state"):
        values.append({key: "PRIVATE_LITERAL"})
    for name in ("id", "version", "count", "active", "ratio", "price", "birthday", "happened_at", "raw"):
        for value in (None, True, -1, 0, 2**63-1, 2**63, 2**64-1, 1.5, -0.0,
                      "12.50", "2024-02-29", "2024-02-29T12:34:56.123+02:00", [], {}):
            values.append({name: value})
    for name in ("birthday", "happened_at"):
        for text in ("2024-2-9", " 2024-02-29", "2023-02-29", "+10000-01-01",
                     "2024-02-29 12:34:56", "2024-02-29T12:34:56Z", "2024-02-29t12:34:56z"):
            values.append({name: text})
    for relation in ("parent", "alternate", "children"):
        for value in (None, {}, [], [None], [1], [True], [{"id": 2}], {"id": 2},
                      {"label": "PRIVATE_LITERAL"}, {"id": 2, "children": []}):
            values.append({"id": 1, relation: value})
    values.extend([
        {"parent": None}, {"children": []}, {"id": None, "children": []},
        {"id": 1, "parent": {"id": 2}},
        {"id": 1, "parent_id": 3, "parent": {"id": 2}},
        {"id": 1, "parent_id": None, "parent": None},
        {"id": 1, "children": [{"id": 2, "parent_id": 99}]},
        {"id": 1, "children": [{"id": 2, "parent_id": 1}]},
        {"id": 1, "children": [{"id": 2, "label": "a"}, {"id": 2, "label": "b"}]},
        {"id": 1, "children": [{"id": 2, "label": "a"}, {"id": 2, "count": 4}]},
        {"id": 1, "parent": {"id": 2, "label": "a"}, "alternate": {"id": 2, "label": "a"}},
        {"id": 1, "parent": {"id": 2, "label": "a"}, "alternate": {"id": 2, "label": "b"}},
        {"id": 1, "children": [{"id": 2, "parent": {"id": 1}}, {"id": 2, "parent_id": 3}]},
        {"label": "first", "stored_label": "second", "unknown": 1},
        {"label": "first", "stored_label": 1},
    ])
    for depth in (64, 65, 66):
        nested = {"id": depth+1}
        for identity in reversed(range(depth)):
            nested = {"id": identity, "parent": nested}
        values.append(nested)
    arrays = [None, {}, [], [{"id": 1}, {"id": 2}],
              [{"id": 1}, {"id": 2, "label": None}, {"id": 3}],
              [{"id": 1, "children": []}, {"id": 2, "children": []}],
              [{"id": 1, "label": "a"}, {"id": 1, "label": "b"}],
              [{"id": 1}, {"fail": True}], [{"id": 1}, True]]
    return [dict(kind=kind, metadata=metadata, registered=registered, array=array, value=value)
            for kind, metadata, registered in itertools.product(range(4), range(4), (False, True))
            for array, collection in ((False, values), (True, arrays)) for value in collection]


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
                  note="Complete original Context decode_json_entity/entities calls: installed descriptors, scalar conversion, operation-local shapes and graph assembly. No generated JSON facade or persistence authority qualification.",
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
            path = ROOT / "docs/parity/json_hydration_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} typed JSON hydration observations differ")
    print(f"Reproduced {len(stimuli)} actual typed JSON hydration observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
