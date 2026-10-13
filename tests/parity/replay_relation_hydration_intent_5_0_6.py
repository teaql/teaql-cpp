#!/usr/bin/env python3
"""Add explicit child query intent to complete historical hydration stimuli."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / "rust_private_save_5_0_6/src/bin/relation_hydration_5_0_6.rs"
HISTORICAL = Path(__file__).with_name("relation_hydration_cases.json")
TARGET = Path(__file__).with_name("relation_hydration_intent_5_0_6_cases.json.gz")


def inputs():
    original = json.loads(HISTORICAL.read_text())
    result = []
    for case in original["cases"]:
        stimulus = copy.deepcopy(case["input"])
        def add(plans):
            for plan in plans:
                query = plan.setdefault("query", {"entity": plan["target"]})
                query["comment"] = "Inspect historical relation hydration"
                query["purpose"] = "Qualify complete original query and graph execution"
                add(plan.get("children", []))
        add(stimulus["plans"])
        result.append(stimulus)
    # Non-ID membership is reusable for both flat cardinalities. ID buckets
    # retain original single-consumption behavior for duplicate owner rows.
    shared = next(v for v in result if v["mode"] == "flat" and
                  v["parents"] == [{"id":{"kind":"u64","value":"1"},"ref":{"kind":"u64","value":"10"}},
                                   {"id":{"kind":"u64","value":"2"},"ref":{"kind":"u64","value":"10"}},
                                   {"id":{"kind":"u64","value":"3"},"ref":{"kind":"u64","value":"11"}}])
    for mode in ("flat", "compact"):
        for inverse_many in (False, True):
            variant = copy.deepcopy(shared)
            variant["mode"] = mode
            variant["plans"][0]["many"] = True
            variant["metadata"][0]["many"] = inverse_many
            result.append(variant)
        variant = copy.deepcopy(shared)
        variant["mode"] = mode
        variant["plans"][0].update(local="id", foreign="parent_id", many=True)
        variant["parents"][1]["id"] = {"kind":"u64","value":"1"}
        variant["responses"][0][0]["parent_id"] = {"kind":"u64","value":"1"}
        variant["metadata"][0].update(local="parent_id", foreign="id")
        result.append(variant)
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
                  historical_corpus_sha256=hashlib.sha256(HISTORICAL.read_bytes()).hexdigest(),
                  note="Separate explicit comment/purpose variants of every historical input. Original inputs remain unchanged. Actual original runtime, query executor and registry, with full output equality.",
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
            path = ROOT / "docs/parity/relation_hydration_intent_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} explicit hydration observations differ")
    print(f"Reproduced {len(stimuli)} explicit original hydration observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
