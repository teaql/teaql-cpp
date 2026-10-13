#!/usr/bin/env python3
"""Replay unchanged historical collection inputs against locked Rust 5.0.6."""

import argparse
import gzip
import json
import subprocess
from pathlib import Path

from compare_values import observe, same


ROOT = Path(__file__).resolve().parents[2]
FAMILIES = ("compact_row_api", "smart_list_api")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust-dir", type=Path, required=True)
    parser.add_argument("--cpp-dir", type=Path)
    parser.add_argument("--check", action="store_true",
                        help="Verify existing versioned corpora without replacing them.")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")],
                   cwd=ROOT, check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    for family in FAMILIES:
        historical = Path(__file__).with_name(family + "_cases.json.gz")
        with gzip.open(historical, "rt", encoding="utf-8") as source:
            old = json.load(source)
        inputs = [case["input"] for case in old["cases"]]
        original = observe(str(args.rust_dir / (family + "_5_0_6")), inputs)
        if args.cpp_dir:
            cpp = observe(str(args.cpp_dir / ("teaql_" + family + "_probe")), inputs)
            differences = [index for index, (expected, actual) in
                           enumerate(zip(original, cpp)) if not same(expected, actual)]
            for index in differences[:3]:
                print(f"{family} mismatch {index}: {inputs[index]}\n"
                      f"Rust: {original[index]}\nC++: {cpp[index]}")
            if differences:
                raise SystemExit(f"{len(differences)}/{len(inputs)} {family} observations differ")
        corpus = dict(
            baseline=lock["commit"], version=lock["version"],
            historical_baseline=old["baseline"],
            note="All historical inputs replayed through an unchanged copy of the original "
                 "public-call driver compiled against locked 5.0.6. Complete outputs, "
                 "no normalization. Historical driver and corpus remain intact.",
            cases=[dict(input=entry, expected=result)
                   for entry, result in zip(inputs, original)],
        )
        target = historical.with_name(family + "_5_0_6_cases.json.gz")
        if args.check:
            with gzip.open(target, "rt", encoding="utf-8") as source:
                committed = json.load(source)
            if not same(corpus, committed):
                raise SystemExit(f"{family} versioned corpus differs from live Rust replay")
        else:
            target.write_bytes(gzip.compress(
                (json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
        suffix = " and C++" if args.cpp_dir else ""
        print(f"Replayed {len(inputs)} {family} observations against actual Rust 5.0.6"
              f"{suffix}", flush=True)


if __name__ == "__main__":
    main()
