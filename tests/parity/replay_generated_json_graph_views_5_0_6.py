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
DRIVER = Path(__file__).parent / "rust_field_layout/src/bin/generated_json_graph_views_5_0_6.rs"
TARGET = Path(__file__).with_name("generated_json_graph_views_5_0_6_cases.json.gz")


def inputs():
    def typed(kind, value):
        return dict(kind=kind, value=value)
    result = []
    for forward, reverse, table, listed, empty, edge, fk, embedded in itertools.product(
        (False, True), (False, True), (False, True), (False, True), (False, True),
        ('absent', 'null', 'target'), (None, 0, 2, 9), (False, True)
    ):
        order = dict(id=typed('u64', '3'), version=typed('i64', '5'), code=typed('text', 'order'))
        if fk is not None:
            order['customer_id'] = typed('u64', str(fk))
        person = dict(id=typed('u64', '2'), version=typed('i64', '7'), name=typed('text', 'person'))
        if embedded:
            order['customer'] = typed('object', person)
        result.append(dict(order=order, person=person, forward_loaded=forward,
                           reverse_loaded=reverse, table=table, list=listed, empty=empty, edge=edge))
    return result


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
                  note="Unchanged original generated Order/Person borrowed JSON over explicit frozen runtime graphs. Tables, selected options/lists, filtered/NULL targets, owned embedded objects and actual address cycles are observed without normalization; no save authority or general model acceptance.",
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
            path = ROOT / "docs/parity/generated_json_graph_views_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} generated typed JSON observations differ")
    print(f"Reproduced {len(stimuli)} actual generated graph presentation observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
