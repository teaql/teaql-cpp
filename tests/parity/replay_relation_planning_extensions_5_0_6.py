#!/usr/bin/env python3
"""Compare actual original relation facet/group-order planning across all helper routes."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / "rust_private_save_5_0_6/src/bin/relation_planning_extensions_5_0_6.rs"
CORPUS = Path(__file__).with_name("relation_planning_extensions_5_0_6_cases.json.gz")


def inputs():
    for groups, aggregate, sliced, order, facets, identity, parents in itertools.product(
            ([], ["owner"], ["owner", "score"], ["owner", "owner"]),
            (False, True), (False, True), range(6), (False, True), (False, True), range(3)):
        yield dict(groups=groups, aggregate=aggregate, slice=sliced, order=order,
                   facets=facets, identity=identity, parents=parents)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    entries = list(inputs())
    original = observe(args.rust, entries)
    for entry, result in zip(entries, original):
        assert result["caller"]["facets"] == int(entry["facets"])
        assert result["caller"]["relations"] == 1
        for route in ("normal", "compact", "base"):
            assert result[route]["facets"] == result[route]["relations"] == 0
            assert result[route]["groups"] == entry["groups"]
        assert same(result["normal"], result["compact"])
    corpus = dict(baseline=lock["commit"], version=lock["version"],
                  driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Actual complete original plan/compact/base helper calls via appended access bridges. Facets, groups, aggregate mode, slice, existing orders, metadata IDs and missing/NULL/valid parents vary independently. Complete outputs; no normalization; original executor panics on planning I/O.",
                  cases=[dict(input=entry, expected=result) for entry, result in zip(entries, original)])
    if args.check:
        with gzip.open(CORPUS, "rt", encoding="utf-8") as source:
            if not same(corpus, json.load(source)):
                raise RuntimeError("Planning extension corpus differs from actual source")
    else:
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    if args.cpp:
        cpp = observe(args.cpp, entries)
        differences = [dict(index=i, input=entry, rust=a, cpp=b)
                       for i, (entry, a, b) in enumerate(zip(entries, original, cpp)) if not same(a, b)]
        if differences:
            report = ROOT / "docs/parity/relation_planning_extensions_5_0_6_initial_gaps.json"
            if not report.exists():
                report.write_text(json.dumps(differences, indent=2) + "\n")
            for difference in differences[:3]:
                print(json.dumps(difference))
            raise SystemExit(f"{len(differences)}/{len(entries)} planning extension observations differ")
    print(f"Reproduced {len(entries)} actual original planning observations" +
          (" and C++" if args.cpp else ""), flush=True)


if __name__ == "__main__":
    main()
