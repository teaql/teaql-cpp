#!/usr/bin/env python3
"""Qualify explicit child intent and sibling aggregate key snapshots with original methods."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
PARITY = Path(__file__).parent
DRIVER = PARITY / "rust_private_save_5_0_6/src/bin/relation_enhancement_variants_5_0_6.rs"
TARGET = PARITY / "relation_enhancement_variants_5_0_6_cases.json.gz"


def inputs():
    original = json.loads((PARITY / "relation_enhancement_cases.json").read_text())["cases"]
    for case in original:
        if case["input"]["mode"] == "child":
            entry = copy.deepcopy(case["input"])
            for enhancement in entry["enhancements"]:
                enhancement["query"].update(comment="Inspect explicit child enhancement", purpose="Verify child row merge")
            yield entry
    base = next(c["input"] for c in original if c["input"]["mode"] == "aggregate")
    for pending in (0, 1):
        for polls in (0, 1, 8):
            for fail_at in (0, 1, 2):
                for missing in (False, True):
                    entry = copy.deepcopy(base)
                    entry.update(pending=pending, polls=polls, fail_at=fail_at)
                    entry["parents"] = [{"id":{"kind":"u64","value":str(i)}} for i in (1, 2)]
                    first = copy.deepcopy(entry["enhancements"][0])
                    first.update(alias="id", single=True)
                    second = copy.deepcopy(first)
                    second["alias"] = "after_shadow"
                    if missing:
                        second["relation"] = "missing"
                    entry["enhancements"] = [first, second]
                    response = [{"parent_col":{"kind":"u64","value":str(i)},
                                 "id":{"kind":"u64","value":str(3-i)}} for i in (1, 2)]
                    entry["responses"] = [response, copy.deepcopy(response)]
                    yield entry


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
        if entry["mode"] == "aggregate" and entry["enhancements"][1]["relation"] == "missing":
            assert not result["calls"]
            assert result.get("cancelled", False) or "error" in result
    corpus = dict(baseline=lock["commit"], version=lock["version"],
                  driver=str(DRIVER.relative_to(ROOT)), driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Historical child stimuli gain explicit comment/purpose. Sibling aggregate variants shadow local ID keys and cover missing relation, suspension/failure/cancel. Actual original method bodies and request intent bridge; recording child-fetch SPI, not SQL/provider qualification.",
                  cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    if args.check:
        with gzip.open(TARGET,"rt",encoding="utf-8") as source:
            assert same(corpus,json.load(source)), "Variant corpus differs from actual source"
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    if args.cpp:
        cpp = observe(args.cpp,entries)
        gaps = [dict(index=i,input=entry,rust=a,cpp=b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
        if gaps:
            p=ROOT/"docs/parity/relation_enhancement_variants_5_0_6_initial_gaps.json"
            if not p.exists():p.write_text(json.dumps(gaps,indent=2)+"\n")
            for gap in gaps[:3]:print(json.dumps(gap))
            raise SystemExit(f"{len(gaps)}/{len(entries)} enhancement variants differ")
    print(f"Reproduced {len(entries)} actual enhancement variants"+(" and C++" if args.cpp else ""),flush=True)


if __name__ == "__main__":
    main()
