#!/usr/bin/env python3
"""Observe original view coordinates, option identity fallback and graph copies."""
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
DRIVER = Path(__file__).parent / "rust_private_save_5_0_6/src/bin/json_graph_views_5_0_6.rs"
TARGET = Path(__file__).with_name("json_graph_views_5_0_6_cases.json.gz")


def inputs():
    result = []
    for pattern, pair, identity, loaded, bad, json_graph, table, shared in itertools.product(
        range(9), ((0, 0), (0, 1), (1, 2), (2**64-1, 0)), (0, 10),
        (False, True), (False, True), (False, True), (False, True), (False, True)
    ):
        a, b = pair
        subject = dict(id=identity, label="first projection " * 20, loaded=loaded, bad_id=bad)
        second = dict(subject, label="second projection " * 20)
        def option(view, relation, value):
            return dict(op="option", view=view, owner="PurchaseOrder", owner_id=1, relation=relation, value=value)
        actions = [option(a, "items", subject)]
        if pattern == 1:
            actions.append(option(b, "other", second))
        elif pattern == 2:
            actions.append(option(a, "items", dict(second, id=identity+1, bad_id=False)))
        elif pattern == 3:
            actions.append(option(a, "items", None))
        elif pattern == 4:
            actions = [dict(op="list", view=a, owner="PurchaseOrder", owner_id=1, relation="items", values=[subject], facets=True)]
        elif pattern == 5:
            actions[0]["op"] = "plain_option"
        elif pattern == 6:
            actions = [dict(op="plain_list", view=a, owner="PurchaseOrder", owner_id=1, relation="items", values=[subject], facets=True)]
        elif pattern == 7:
            actions.append(option(b, "other", dict(second, loaded=False)))
        elif pattern == 8:
            actions.append(option(a, "items", dict(second, bad_id=True)))
        if table:
            actions.append(dict(op="table", value=dict(subject, label="ordinary table", bad_id=False)))
        lookups = [dict(view=view, owner=owner, owner_id=1, relation=relation, id=id)
                   for view, owner, relation, id in itertools.product(
                       sorted(set((a, b, 0, 999))), ("PurchaseOrder", "purchase_order"),
                       ("items", "other"), (identity, identity+1))]
        result.append(dict(json=json_graph, actions=actions, lookups=lookups, shared_ledger=shared,
                           control=dict(pattern=pattern)))
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
                  note="Complete original JSON view keyed graph storage, option identity index/fallback, table priority, facet isolation and runtime view/ledger copy methods. Repeated/replaced edge behavior retains source results. No JSON parser/typed JSON registry execution qualification.",
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
            path = ROOT / "docs/parity/json_graph_views_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} graph view observations differ")
    print(f"Reproduced {len(stimuli)} actual graph view observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
