#!/usr/bin/env python3
"""Observe original graph facet storage independently of query-only embedded rows."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / "rust_field_layout/src/bin/graph_relation_facets_5_0_6.rs"
TARGET = Path(__file__).with_name("graph_relation_facets_5_0_6_cases.json.gz")


def inputs():
    result = []
    for owner, op, sequence, owner_id, populated in itertools.product(
        ("PurchaseOrder", "purchase_order", "purchaseOrder"),
        ("facets", "list"), range(3), (0, 1), (False, True)
    ):
        def action(kind, data):
            return dict(op=kind, owner=owner, id=owner_id, relation="reference", facets=data)
        first = {"choices": ["long shared value " * 20] if populated else []}
        actions = [action(op, first)]
        if sequence == 1:
            actions.append(action("facets", {}))
        if sequence == 2:
            actions.extend([action("facets", {"replacement": ["second"]}), action("list", {})])
        actions.append(dict(op="null", owner=owner, id=owner_id, relation="optional"))
        lookups = [dict(owner=lookup_owner, id=identifier, relation=relation, facet=facet)
                   for lookup_owner, identifier, relation, facet in itertools.product(
                       (owner, "purchase_order", "Other"), (owner_id, owner_id + 2),
                       ("reference", "missing"), ("choices", "replacement", "missing"))]
        result.append(dict(actions=actions, lookups=lookups, snapshot_facets=first))
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
    output = observe(args.rust, stimuli)
    corpus = dict(baseline=lock["commit"], version=lock["version"],
                  driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Actual public original graph builder/freeze/relation_facet and fresh ledger APIs. No query execution or JSON view qualification.",
                  cases=[dict(input=i, expected=o) for i, o in zip(stimuli, output)])
    if args.check:
        with gzip.open(TARGET, "rt", encoding="utf-8") as source:
            assert same(corpus, json.load(source)), "Corpus differs from actual source"
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    if args.cpp:
        actual = observe(args.cpp, stimuli)
        gaps = [dict(index=n, input=i, rust=a, cpp=b)
                for n, (i, a, b) in enumerate(zip(stimuli, output, actual)) if not same(a, b)]
        if gaps:
            path = ROOT / "docs/parity/graph_relation_facets_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} graph observations differ")
    print(f"Reproduced {len(stimuli)} actual graph observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
