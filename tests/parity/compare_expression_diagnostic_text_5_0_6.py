#!/usr/bin/env python3
"""Compare original raw diagnostics with explicit C++ rendering adaptations."""
import argparse,gzip,hashlib,json,re,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
LABELS=["original_expr_with_broken_point","root","access_path","break_point","missing_preload","suggested_fix","nested_fix","severity","human_message"]
def fields(raw):
    markers=["\n"+name+": " for name in LABELS]
    positions=[]; start=0
    for marker in markers:
        position=raw.find(marker,start)
        if position<0: raise ValueError("Original diagnostic marker missing: "+marker)
        positions.append(position);start=position+len(marker)
    return {name:raw[positions[i]+len(markers[i]):positions[i+1] if i+1<len(positions) else len(raw)].removesuffix("\n") if i+1==len(positions) else raw[positions[i]+len(markers[i]):positions[i+1]] for i,name in enumerate(LABELS)}
def method(prefix,name):
    # Explicit ASCII C++ identifier casing; opaque Unicode is preserved.
    out=prefix; upper=bool(prefix)
    for c in name:
        if c=="_":upper=True
        else:
            out+=chr(ord(c)-32) if upper and "a"<=c<="z" else c
            upper=False
    return out
def camel(name):
    parts=name.split("_")
    return parts[0]+"".join((chr(ord(p[0])-32) if "a"<=p[0]<= "z" else p[0])+p[1:] for p in parts[1:] if p)
def adapt(raw,input):
    result=fields(raw); node=input["node"];path=input["path"].split(".")
    original=result["original_expr_with_broken_point"][1:-1]
    end=original.index("("); original="E::"+camel(original[3:end])+original[end:]
    for part in sorted(set(path),key=len,reverse=True):
        original=original.replace(".get_"+part+"()","."+method("get",part)+"()")
    result["original_expr_with_broken_point"]='"'+original+'"'
    result["suggested_fix"]='"'+result["suggested_fix"][1:-1].replace("select_"+node,method("select",node),1)+'"'
    if result["nested_fix"]!="null":
        nested=result["nested_fix"][1:-1]
        names=sorted(set(path+[node]),key=len,reverse=True)
        for part in names:
            nested=nested.replace("select_"+part+"(",method("select",part)+"With(")
            nested=nested.replace(method("select",part)+"With()",method("select",part)+"()")
        result["nested_fix"]='"'+nested+'"'
    message="Access to "+input["root"].split("(")[0]+"."+input["path"]+" requires preloading. Add "+result["suggested_fix"][1:-1]
    if result["nested_fix"]!="null":message+=" or the complete nested selection "+result["nested_fix"][1:-1]
    result["human_message"]='"'+message+' to the query."'
    return result
def native_fields(value):
    def array(items):return "["+", ".join('"'+x+'"' for x in items)+"]"
    return {k:value[k] if k=="root" else array(value[k]) if k in ("access_path","missing_preload") else "null" if value[k] is None else '"'+value[k]+'"' for k in LABELS}
def main():
    p=argparse.ArgumentParser();p.add_argument("--cpp",required=True);p.add_argument("--rust");a=p.parse_args()
    with gzip.open(Path(__file__).with_name("expression_diagnostic_text_5_0_6_cases.json.gz"),"rt") as f: corpus=json.load(f)
    assert corpus["baseline"]==json.loads((ROOT/"upstream.lock.json").read_text())["commit"]
    assert corpus["source_sha256"]==hashlib.sha256((ROOT/"tests/generator/parity/dynamic_json_rust/lib/src/e.rs").read_bytes()).hexdigest()
    assert corpus["producer"]==json.loads((ROOT/"tests/generator/parity/dynamic_json_rust/PROVENANCE.json").read_text())
    cases=[x["input"] for x in corpus["cases"]];raw=[x["expected"] for x in corpus["cases"]]
    if a.rust:assert same(raw,observe(a.rust,cases)),"Original raw diagnostics changed"
    cpp=observe(a.cpp,cases)
    for i,(x,r,c) in enumerate(zip(cases,raw,cpp)):
        if not same(adapt(r["raw"],x),native_fields(c)):raise SystemExit(f"Diagnostic case {i} differs: {x}\nSource: {adapt(r['raw'],x)}\nC++: {native_fields(c)}")
    print(f"Compared {len(cases)} actual original/native diagnostic text outcomes (explicit ASCII casing and English adaptation)")
if __name__=="__main__":main()
