#!/usr/bin/env python3
"""Observe unchanged original event/privacy methods and exact native results."""
import argparse, gzip, hashlib, itertools, json, subprocess
from pathlib import Path
from compare_values import observe, same
ROOT = Path(__file__).resolve().parents[2]
def inputs():
    old = dict(kind="text", value="PRIVATE-OLD")
    values = [dict(kind="absent"), dict(kind="null"), dict(kind="typed_null"),
        dict(kind="text", value="PRIVATE-NEW"), dict(kind="text", value="雪😀e\u0301\nPRIVATE-NEW"),
        dict(kind="bool", value=False), dict(kind="i64", value="-9223372036854775808"),
        dict(kind="u64", value="18446744073709551615"), dict(kind="f64", value="-0.0"),
        dict(kind="decimal", value="12345.6700"), dict(kind="date", value="2026-10-13"),
        dict(kind="timestamp", value="-1"), dict(kind="json", value={"password":"PRIVATE-NEW","note":"visible"}),
        dict(kind="object", value={"api_key":dict(kind="text",value="PRIVATE-NEW"),"note":dict(kind="text",value="visible")}),
        dict(kind="list", value=[dict(kind="text",value="PRIVATE-NEW"),dict(kind="null")]),
        dict(kind="text",value="雪"*2048+"PRIVATE-NEW")]
    pairs = [(old, value) for value in values] + [(value, dict(kind="text",value="PRIVATE-NEW")) for value in values[:-1]]
    return [dict(old=old,new=new,field=field,allow=allow,provenance=provenance,masked=masked,limit=limit)
        for (old,new),field,allow,provenance,masked,limit in itertools.product(pairs,
            ("#extension","#password","ordinary","_private"),(False,True),(False,True),(False,True),(None,0,3,8))]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust",required=True); parser.add_argument("--cpp",required=True)
    parser.add_argument("--check",action="store_true"); args=parser.parse_args()
    subprocess.run(["python3",str(ROOT/"scripts/verify_upstream.py")],check=True)
    cases=inputs(); rust=observe(args.rust,cases); cpp=observe(args.cpp,cases)
    differences=[dict(index=i,input=x,rust=a,cpp=b) for i,(x,a,b) in enumerate(zip(cases,rust,cpp)) if not same(a,b)]
    if differences:
        destination=ROOT/"docs/parity/dynamic_audit_projection_5_0_6_initial_gaps.json"
        if not destination.exists(): destination.write_text(json.dumps(dict(count=len(differences),differences=differences),indent=2)+"\n")
        print(json.dumps(differences[:1],indent=2))
        raise SystemExit(f"{len(differences)}/{len(cases)} dynamic audit observations differ")
    sources=["teaql-runtime/src/event.rs","teaql-runtime/src/log_privacy.rs"]
    corpus=dict(baseline=json.loads((ROOT/"upstream.lock.json").read_text())["commit"],
        sources={p:hashlib.sha256((ROOT/".reference/teaql-rs"/p).read_bytes()).hexdigest() for p in sources},
        note="Exact original event/privacy modules; dynamic field Set/NULL/absence projection, credentials, Unicode and truncation. Not durable provider/save execution evidence.",
        cases=[dict(input=x,expected=y) for x,y in zip(cases,rust)])
    target=Path(__file__).with_name("dynamic_audit_projection_5_0_6_cases.json.gz")
    if args.check:
        with gzip.open(target,"rt") as f: assert same(corpus,json.load(f)), "Stored source evidence changed"
    else: target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    print(f"Compared {len(cases)} actual original/native dynamic audit projections")
if __name__=="__main__": main()
