#!/usr/bin/env python3
"""Observe original typed JSON registry capability and edge decoder callbacks."""
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
DRIVER = Path(__file__).parent / "rust_private_save_5_0_6/src/bin/json_graph_decoder_5_0_6.rs"
TARGET = Path(__file__).with_name("json_graph_decoder_5_0_6_cases.json.gz")


def inputs():
    def value(kind, payload):
        return dict(kind=kind, value=payload)

    def row(view, identity="10", fail=False):
        fields = dict(id=value("u64", identity), label=value("text", "projection " * 20),
                      version=value("i64", "1"))
        if fail:
            fields["fail"] = value("bool", True)
        return dict(view=view, fields=fields)

    patterns = [[], [row(1)], [row(1), row(2, "11")],
                [row(1), row(2)], [row(1, fail=True)],
                [row(1), row(2, fail=True)],
                [dict(view=1, fields=dict(label=value("text", "without identity")))],
                [row(2**64-1)]]
    return [dict(kind=kind, metadata=metadata, registered=registered, copy=copied,
                 many=many, json=json_graph, parent_view=parent, entity=entity, rows=rows)
            for kind, metadata, registered, copied, many, json_graph, parent, rows
            in itertools.product(range(4), range(12), (False, True), (False, True),
                                 (False, True), (False, True), (0, 999), patterns)
            for entity in ("Child", "Missing")]


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
                  note="Original typed JSON registry callbacks: descriptor validation, layout capability, callback copies, list/option edges, child views and weak graph state. Full JSON input parsing remains unqualified.",
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
            path = ROOT / "docs/parity/json_graph_decoder_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} graph view observations differ")
    print(f"Reproduced {len(stimuli)} actual typed JSON decoder observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
