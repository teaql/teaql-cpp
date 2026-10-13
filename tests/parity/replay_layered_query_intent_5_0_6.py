#!/usr/bin/env python3
"""Replay historical layered query stimuli with explicitly supplied purpose variants."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
PARITY = Path(__file__).parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust-dir", type=Path, required=True)
    parser.add_argument("--cpp-dir", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    for family in ("runtime_base", "context_query"):
        historical = PARITY / (family + "_cases.json")
        old = json.loads(historical.read_text())
        inputs = [dict(case["input"], purpose=purpose)
                  for case in old["cases"]
                  for purpose in ("Inspect layered query intent", " ", "  Inspect layered query intent  ")]
        driver = PARITY / "rust_private_save_5_0_6/src/bin" / (family + "_intent_5_0_6.rs")
        original = observe(str(args.rust_dir / (family + "_intent_5_0_6")), inputs)
        corpus = dict(baseline=lock["commit"], version=lock["version"],
                      historical_baseline=old["baseline"],
                      historical_corpus_sha256=hashlib.sha256(historical.read_bytes()).hexdigest(),
                      driver=str(driver.relative_to(ROOT)),
                      driver_sha256=hashlib.sha256(driver.read_bytes()).hexdigest(),
                      note="Every original historical stimulus, independently expanded with explicit valid/blank/padded purpose. Existing verbatim locked method fragments and SPI spies remain; no new output normalization.",
                      cases=[dict(input=entry, expected=result) for entry, result in zip(inputs, original)])
        target = PARITY / (family + "_intent_5_0_6_cases.json.gz")
        if args.check:
            with gzip.open(target, "rt", encoding="utf-8") as source:
                if not same(corpus, json.load(source)):
                    raise RuntimeError(f"{family}: purpose corpus differs from actual source")
        else:
            target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
        cpp = observe(str(args.cpp_dir / ("teaql_" + family + "_probe")), inputs)
        differences = [dict(index=i, input=entry, rust=a, cpp=b)
                       for i, (entry, a, b) in enumerate(zip(inputs, original, cpp)) if not same(a, b)]
        if differences:
            gap = ROOT / "docs/parity" / (family + "_intent_5_0_6_initial_gaps.json")
            if not gap.exists():
                gap.write_text(json.dumps(differences, indent=2) + "\n")
            for difference in differences[:3]:
                print(json.dumps(difference))
            raise SystemExit(f"{len(differences)}/{len(inputs)} {family} purpose observations differ")
        print(f"Compared and reproduced {len(inputs)} actual {family} explicit purpose observations", flush=True)


if __name__ == "__main__":
    main()
