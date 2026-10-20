#!/usr/bin/env python3
"""Compare original generated Product audited saves with real SQLite storage."""
import argparse, gzip, hashlib, itertools, json, subprocess
from pathlib import Path
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    return [dict(op=op,trigger=trigger,audit_fail=audit_fail,conflict=conflict) for op,trigger,audit_fail,conflict in itertools.product(("set","null","delete"),(False,True),(False,True),(False,True))]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--rust",required=True);p.add_argument("--cpp",required=True);p.add_argument("--check",action="store_true");a=p.parse_args()
    subprocess.run(["python3",str(ROOT/"scripts/verify_upstream.py")],check=True)
    cases=inputs(); rust=observe(a.rust,cases);cpp=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=y,cpp=z) for i,(x,y,z) in enumerate(zip(cases,rust,cpp)) if not same(y,z)]
    if gaps:
        target=ROOT/"docs/parity/dynamic_save_audit_5_0_6_initial_gaps.json"
        if not target.exists(): target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+"\n")
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f"{len(gaps)}/{len(cases)} dynamic save/audit observations differ")
    source=json.loads((ROOT/"tests/generator/parity/dynamic_json_rust/PROVENANCE.json").read_text())
    corpus=dict(baseline=json.loads((ROOT/"upstream.lock.json").read_text())["commit"],producer=source,note="Actual unchanged same-model generated Product Q and audited save. SQLite storage/version, complete safe audit, exact ordered SQL/parameter/intent projections and ledger clearing. Clock/timing fields are omitted symmetrically as nondeterministic observations; no output normalization.",cases=[dict(input=x,expected=y) for x,y in zip(cases,rust)])
    target=Path(__file__).with_name("dynamic_save_audit_5_0_6_cases.json.gz")
    if a.check:
        with gzip.open(target,"rt") as f: assert same(corpus,json.load(f)),"Evidence differs from live source"
    else: target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    print(f"Compared {len(cases)} actual source/native durable dynamic save/audit observations")
if __name__=="__main__":main()
