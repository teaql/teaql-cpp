#!/usr/bin/env python3
"""Replay complete historical input families through separately built 5.0.6 drivers."""

import argparse
import gzip
import hashlib
import json
import re
import subprocess
from pathlib import Path

from compare_values import observe, same


ROOT = Path(__file__).resolve().parents[2]


def load(path):
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", encoding="utf-8") as source:
        return json.load(source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", action="append", required=True)
    parser.add_argument("--rust-dir", type=Path, required=True)
    parser.add_argument("--driver-dir", type=Path,
                        default=Path(__file__).parent / "rust_field_layout/src/bin",
                        help="Versioned observer sources; alternate private-method crates are allowed.")
    parser.add_argument("--observer-family",
                        help="Use one actual observer for multiple historical input families.")
    parser.add_argument("--cpp-observer-family",
                        help="Use a separately named C++ probe for a shared Rust observer.")
    parser.add_argument("--cpp-dir", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    args.driver_dir = args.driver_dir.resolve()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    for family in args.family:
        if re.fullmatch(r"[a-z][a-z0-9_]*", family) is None:
            raise ValueError(f"Invalid family name: {family}")
        observer = args.observer_family or family
        if re.fullmatch(r"[a-z][a-z0-9_]*", observer) is None:
            raise ValueError(f"Invalid observer name: {observer}")
        historical = Path(__file__).with_name(family + "_cases.json.gz")
        if not historical.is_file():
            historical = historical.with_suffix("")
        old = load(historical)
        inputs = [case["input"] for case in old["cases"]]
        driver = args.driver_dir / (observer + "_5_0_6.rs")
        original = observe(str(args.rust_dir / (observer + "_5_0_6")), inputs)
        if args.cpp_dir:
            cpp_observer = args.cpp_observer_family or observer
            if re.fullmatch(r"[a-z][a-z0-9_]*", cpp_observer) is None:
                raise ValueError(f"Invalid C++ observer name: {cpp_observer}")
            cpp = observe(str(args.cpp_dir / ("teaql_" + cpp_observer + "_probe")), inputs)
            failures = [i for i, (a, b) in enumerate(zip(original, cpp)) if not same(a, b)]
            for i in failures[:3]:
                print(f"{family} case {i}: {inputs[i]}\nRust: {original[i]}\nC++: {cpp[i]}")
            if failures:
                raise SystemExit(f"{len(failures)}/{len(inputs)} {family} observations differ")
        corpus = dict(
            baseline=lock["commit"], version=lock["version"],
            historical_baseline=old["baseline"],
            historical_corpus_sha256=hashlib.sha256(historical.read_bytes()).hexdigest(),
            driver=str(driver.relative_to(ROOT)),
            driver_sha256=hashlib.sha256(driver.read_bytes()).hexdigest(),
            note="All historical inputs in their original order, replayed through separately "
                 "compiled actual locked 5.0.6 calls. Complete outputs, no normalization. "
                 "Historical corpus/driver remain intact. Driver adaptations and semantic "
                 "qualification scope must be reviewed independently.",
            cases=[dict(input=entry, expected=result) for entry, result in zip(inputs, original)],
        )
        target = historical.with_name(family + "_5_0_6_cases.json.gz")
        if args.check:
            if not same(corpus, load(target)):
                raise SystemExit(f"{family} corpus differs from live source evidence")
        else:
            target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
        suffix = " and C++" if args.cpp_dir else ""
        print(f"Replayed {len(inputs)} actual {family} Rust 5.0.6 observations{suffix}", flush=True)


if __name__ == "__main__":
    main()
