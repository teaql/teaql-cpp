#!/usr/bin/env python3
"""Retain actual unparsed producer panics for adversarial diagnostic strings."""
import argparse,gzip,hashlib,itertools,json,subprocess
from pathlib import Path
from compare_values import observe,same
from compare_expression_diagnostic_text_5_0_6 import adapt,native_fields
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    roots=["Person(id=7)","Root_Name(id=7)","Person(id=(7))","Person((value)(nested))","Person(","Person","",'Person(id="7")',"Person(id=\\7)","Person(id=\u0000)","Person(id=7) trailing","École(id=7)","Root\nLabel(id=7)"]
    nodes=["name","first_name",'quote"name',"slash\\name","tab\tname","line\nname","nul\u0000name","雪","e\u0301","field name","hyphen-name",""]
    return [dict(root=root,node=node,path=path) for root,node in itertools.product(roots,nodes) for path in [node,"parent."+node,node+".child.name",node+"."+node+".name","missing..name",""]]
def main():
    p=argparse.ArgumentParser();p.add_argument("--rust",required=True);p.add_argument("--cpp",required=True);p.add_argument("--check",action="store_true");a=p.parse_args()
    subprocess.run(["python3",str(ROOT/"scripts/verify_upstream.py")],check=True)
    cases=inputs(); raw=observe(a.rust,cases);cpp=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=r,cpp=c) for i,(x,r,c) in enumerate(zip(cases,raw,cpp)) if not same(adapt(r["raw"],x),native_fields(c))]
    if gaps:
        target=ROOT/"docs/parity/expression_diagnostic_text_5_0_6_initial_gaps.json"
        if not target.exists():target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+"\n")
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f"{len(gaps)}/{len(cases)} diagnostic text outcomes differ")
    source=ROOT/"tests/generator/parity/dynamic_json_rust/lib/src/e.rs"
    corpus=dict(baseline=json.loads((ROOT/"upstream.lock.json").read_text())["commit"],source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),producer=json.loads((source.parents[2]/"PROVENANCE.json").read_text()),notice="Original panics retained verbatim, including invalid unescaped quotation/control characters. Structured native fields compare through explicit ASCII API casing and English text adaptation. Cases exclude text containing the fixed field-label delimiter sequence; no generic untrusted panic parser claim.",cases=[dict(input=x,expected=r) for x,r in zip(cases,raw)])
    target=Path(__file__).with_name("expression_diagnostic_text_5_0_6_cases.json.gz")
    if a.check:
        with gzip.open(target,"rt") as f:assert same(corpus,json.load(f))
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    print(f"Compared {len(cases)} original/native diagnostic text cases")
if __name__=="__main__":main()
