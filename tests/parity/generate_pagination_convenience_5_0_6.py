#!/usr/bin/env python3
"""Compare actual generated pagination modifiers and their SQLite execution."""
import argparse,gzip,hashlib,itertools,json,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    maximum=18446744073709551615
    def op(name,number=0,size=0):return dict(op=name,number=number,size=size)
    operations=[op(name,n,size) for name,(n,size) in itertools.product(
        ("limit","skip","offset_only","offset","page_offset","offset_size","top","page_number","page_number_default","page","page_default","unlimited"),
        ((0,0),(0,1),(1,0),(1,1),(2,2),(maximum,2),(maximum,maximum)))]
    sequences=[[x] for x in operations]+[
        [op("offset",2,1),op("unlimited"),op("skip",1),op("top",size=2)],
        [op("limit",size=1),op("offset_only",2),op("page_number",maximum,0)],
        [op("page_default",maximum),op("top",size=0)],
        [op("page_number",maximum,maximum),op("skip",0)]]
    return [dict(entity=entity,minimal=minimal,filters=["{}"],return_type="native",offset=0,limit=1,missing_table=False,optimization="none",max_ids=100,
        mode=mode,pagination=sequence,after_purpose=stage)
        for sequence,entity,minimal,mode,stage in itertools.product(sequences,("person","product","order","line"),(False,True),("count","exists","first","one","list","page"),(False,True))]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--rust",required=True);p.add_argument("--cpp",required=True);p.add_argument("--check",action="store_true");a=p.parse_args()
    subprocess.run(["python3",str(ROOT/"scripts/verify_upstream.py")],check=True)
    cases=inputs();rust=observe(a.rust,cases);cpp=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=y,cpp=z) for i,(x,y,z) in enumerate(zip(cases,rust,cpp)) if not same(y,z)]
    if gaps:
        target=ROOT/"docs/parity/pagination_convenience_5_0_6_initial_gaps.json"
        if not target.exists():target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+"\n")
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f"{len(gaps)}/{len(cases)} pagination convenience outcomes differ")
    corpus=dict(baseline=json.loads((ROOT/"upstream.lock.json").read_text())["commit"],producer=json.loads((ROOT/"tests/generator/parity/dynamic_json_rust/PROVENANCE.json").read_text()),
        note="Actual unmodified generated pagination APIs and typed SQLite count/exists/first/one/list/page. Four models, full/minimal projections, saturated geometry and modifier composition. C++ before/after purpose; original public inner/re-purpose adaptation after staging. Exact SQL/params/results/errors, no normalization.",cases=[dict(input=x,expected=y) for x,y in zip(cases,rust)])
    target=Path(__file__).with_name("pagination_convenience_5_0_6_cases.json.gz")
    if a.check:
        with gzip.open(target,"rt") as f:assert same(corpus,json.load(f)),"Original evidence changed"
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    print(f"Compared {len(cases)} actual source/native pagination convenience outcomes")
if __name__=="__main__":main()
