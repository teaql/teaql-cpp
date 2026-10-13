#!/usr/bin/env python3
"""Replay complete original per-owner facet futures with delay, failure and cancellation."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]
DRIVER=Path(__file__).parent/"rust_private_save_5_0_6/src/bin/hydration_owner_facets_async_5_0_6.rs"
TARGET=Path(__file__).with_name("hydration_owner_facets_async_5_0_6_cases.json.gz")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust",required=True);parser.add_argument("--cpp");parser.add_argument("--check",action="store_true")
    args=parser.parse_args();subprocess.run(["python3",str(ROOT/"scripts/verify_upstream.py")],check=True)
    lock=json.loads((ROOT/"upstream.lock.json").read_text())
    inputs=[dict(facets=True,include_all=a,sliced=True,many=m,keys=k,pending=p,polls=b,fail_at=f) for a,m,k,p,b,f in itertools.product((False,True),(False,True),range(3),(0,1,3),(0,1,2,20),range(3))]
    outputs=observe(args.rust,inputs)
    corpus=dict(baseline=lock["commit"],version=lock["version"],driver=str(DRIVER.relative_to(ROOT)),driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
        note="Complete original embedded per-owner facet futures with delay/drop/error/cancel budgets. Full parent/facet/request/error/stop snapshots. Recording executor, not native-thread or physical SQL qualification.",cases=[dict(input=i,expected=o) for i,o in zip(inputs,outputs)])
    if args.check:
        with gzip.open(TARGET,"rt",encoding="utf-8") as source: assert same(corpus,json.load(source)),"Corpus differs from actual source"
    else:TARGET.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    if args.cpp:
        cpp=observe(args.cpp,inputs)
        gaps=[dict(index=n,input=i,rust=a,cpp=b) for n,(i,a,b) in enumerate(zip(inputs,outputs,cpp)) if not same(a,b)]
        if gaps:
            path=ROOT/"docs/parity/hydration_owner_facets_async_5_0_6_initial_gaps.json"
            if not path.exists():path.write_text(json.dumps(gaps,indent=2)+"\n")
            for g in gaps[:2]:print(json.dumps(g))
            raise SystemExit(f"{len(gaps)}/{len(inputs)} hydration observations differ")
    print(f"Reproduced {len(inputs)} actual hydration observations"+(" and C++" if args.cpp else ""),flush=True)
if __name__=="__main__":main()
