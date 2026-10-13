#!/usr/bin/env python3
"""Qualify independent SQL capture options through actual public entity queries."""

import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name("entity_query_capture_5_0_6_cases.json.gz")
DRIVER = Path(__file__).with_name("rust_private_save_5_0_6") / "src/bin/entity_query_capture_5_0_6.rs"


def inputs():
    for mode, select, mutation, continuous, failure, comment, aggregate, cached in itertools.product(
            ("list", "owned", "rows", "aggregates"), (False, True), (False, True),
            (False, True), (False, True), (None, "business", ""),
            (False, True), (False, True)):
        yield dict(mode=mode, select_capture=select, mutation_capture=mutation,
                   capture=True, continuous=continuous, failure=failure, comment=comment,
                   aggregate=aggregate, cache=cached, backend=cached,
                   limit=1, calls=2, purpose="Inspect independent SQL capture")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    entries = list(inputs())
    original = observe(args.rust, entries)
    by_options = {}
    for entry, result in zip(entries, original):
        # Source behavior must demonstrate the capture dimension being qualified.
        for request in result["requests"]:
            assert request["capture"] == entry["select_capture"]
            assert request["debug"] == entry["select_capture"]
            if entry["mode"] == "list":
                assert not request["query_trace"]
                assert bool(request["request_trace"]) == entry["select_capture"]
        if entry["mode"] == "list" and result["requests"]:
            assert len(result["requests"]) == entry["calls"]
        key = json.dumps({k: v for k, v in entry.items() if k != "mutation_capture"},
                         sort_keys=True)
        if key in by_options:
            assert same(by_options[key], result), "Mutation capture changed SELECT behavior"
        else:
            by_options[key] = result
    corpus = dict(baseline=lock["commit"], version=lock["version"],
                  driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Actual public EntityDataService calls with independently explicit SELECT/MUTATION log options. Complete outputs without new normalization. Historical observers remain intact.",
                  cases=[dict(input=entry, expected=result)
                         for entry, result in zip(entries, original)])
    if args.check:
        with gzip.open(CORPUS, "rt", encoding="utf-8") as source:
            if not same(corpus, json.load(source)):
                raise RuntimeError("Capture corpus differs from live locked source")
    else:
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    cpp = observe(args.cpp, entries)
    differences = [dict(index=i, input=entry, rust=a, cpp=b)
                   for i, (entry, a, b) in enumerate(zip(entries, original, cpp)) if not same(a, b)]
    if differences:
        report = ROOT / "docs/parity/entity_query_capture_5_0_6_initial_gaps.json"
        if not report.exists():
            report.write_text(json.dumps(differences, indent=2) + "\n")
        for difference in differences[:3]:
            print(json.dumps(difference, ensure_ascii=False))
        raise SystemExit(f"{len(differences)}/{len(entries)} capture observations differ")
    requests = [request for result in original for request in result["requests"]]
    print(f"Compared and reproduced {len(entries)} actual SQL capture observations; "
          f"{len(requests)} provider calls, "
          f"{sum(request['capture'] for request in requests)} captured requests", flush=True)


if __name__ == "__main__":
    main()
