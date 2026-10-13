#!/usr/bin/env python3
"""Reproduce complete original loaded-row graph reconstruction and read metadata."""
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
DRIVER = Path(__file__).parent / "rust_private_save_5_0_6/src/bin/loaded_row_graph_5_0_6.rs"
TARGET = Path(__file__).with_name("loaded_row_graph_5_0_6_cases.json.gz")


def v(kind, value=None):
    return dict(kind=kind, **({} if value is None else dict(value=value)))


def u(value):
    return v("u64", str(value))


def row(fields, loaded=None):
    return dict(fields=fields, loaded=loaded or {})


def smart(data, facets=None):
    return dict(data=data, total_count=123, is_loaded=False,
                summary={"reason": v("text", "original relation summary")},
                aggregations={"metric": v("i64", "9")}, facets=facets or {})


def inputs():
    result = []
    for many, reverse, payload, cache, with_facets, nested, registered, owner in itertools.product(
        (False, True), (False, True), range(7), range(3),
        (False, True), (False, True), (False, True), range(3)
    ):
        local, foreign = ("id", "owner") if reverse else ("link", "id")
        leaf = row({"id": u(100), "owner": u(10), "label": v("text", "nested leaf")})
        facet = smart([row({"id": u(90), "label": v("text", "reused facet payload " * 20)})])
        facets = {"choices": facet} if with_facets else {}
        child_fields = {"id": u(10), "owner": u(1), "label": v("text", "first projection")}
        child_loaded = {}
        if nested:
            child_fields["leaves"] = v("list", [v("object", leaf["fields"])])
            child_loaded["leaves"] = smart([leaf], facets)
        child = row(child_fields, child_loaded)
        payload_value = [v("object", child_fields), v("list", [v("bool", False), v("object", child_fields)]),
                         v("null"), v("typed_null"), v("text", "scalar relation"),
                         None, v("object", {foreign: u(10)})][payload]
        fields = {"id": u(1)}
        if owner == 1:
            fields.pop("id")
        if owner == 2:
            fields["id"] = v("text", "invalid owner")
        if payload_value is not None:
            fields["children"] = payload_value
        loaded = {"children": smart([child] if cache == 1 else [], facets)} if cache else {}
        result.append(dict(many=many, local=local, foreign=foreign, registered=registered,
                           rows=[row(fields, loaded)], control=dict(payload=payload, cache=cache,
                                with_facets=with_facets, nested=nested, owner=owner)))
    # Shared identities retain the first installed table projection; filtered
    # identity-only references must remain edge-owned even beside a full sibling.
    for order, foreign in itertools.product((False, True), ("id", "owner")):
        full = row({"id": u(1), "children": v("object", {"id": u(10), "owner": u(10), "label": v("text", "visible")})})
        identity = row({"id": u(2), "children": v("object", {foreign: u(10)})})
        result.append(dict(many=False, local="link", foreign=foreign, registered=True,
                           rows=[identity, full] if order else [full, identity], control=dict(sibling=True)))
    for many, reverse, failure in itertools.product((False, True), (False, True), ("decode", "missing_id", "bad_id")):
        fields = {"id": u(10), "owner": u(1), "label": v("text", "error projection")}
        if failure == "decode":
            fields["fail"] = v("bool", True)
        elif failure == "missing_id":
            fields.pop("id")
        else:
            fields["id"] = v("text", "invalid child")
        result.append(dict(many=many, local="id" if reverse else "link", foreign="owner",
                           registered=True, rows=[row({"id": u(1), "children": v("object", fields)},
                            {"children": smart([row(fields)], {"choices": smart([])})})],
                           control=dict(failure=failure)))
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
    corpus = dict(baseline=lock["commit"], version=lock["version"],
                  driver=str(DRIVER.relative_to(ROOT)), driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Complete original attach/flatten loaded graph and actual registry. Rows, shape markers, loaded caches, facet/full list metadata, identity/edge views and exact errors; no protected I/O.",
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
            target = ROOT / "docs/parity/loaded_row_graph_5_0_6_initial_gaps.json"
            if not target.exists():
                target.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} reconstruction observations differ")
    print(f"Reproduced {len(stimuli)} actual graph reconstruction observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
