#!/usr/bin/env python3
"""Compare original generated Product audited saves with real SQLite storage."""
import argparse, gzip, hashlib, itertools, json, subprocess
from pathlib import Path
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    def scalar(kind,value): return dict(kind=kind,value=value)
    profiles=[
        (0,scalar("bool",False),scalar("bool",True)),
        (1,scalar("i64","-9223372036854775808"),scalar("i64","9223372036854775807")),
        (2,scalar("u64","18446744073709551615"),scalar("u64","0")),
        (3,scalar("f64","-0.0"),scalar("f64","2.25")),
        (4,scalar("decimal","12345.6700"),scalar("decimal","-9876.5400")),
        (5,scalar("text","PRIVATE-OLD"),scalar("text","PRIVATE-NEW")),
        (6,scalar("text","PRIVATE-OLD雪"*128),scalar("text","PRIVATE-NEW😀"*128)),
        (7,scalar("json",{"note":"PRIVATE-OLD","nested":[None,False]}),scalar("json",{"note":"PRIVATE-NEW","password":"PRIVATE-TOKEN"})),
        (8,scalar("date","2020-02-29"),scalar("date","1969-12-31")),
        (9,scalar("timestamp","-9223372036854775808"),scalar("timestamp","9223372036854775807")),
        (5,dict(kind="null"),scalar("text","PRIVATE-NEW")),
        (5,scalar("text","PRIVATE-OLD"),dict(kind="typed_null"))]
    return [dict(type=type,stored=stored,value=value,op=op,trigger=trigger,audit_fail=audit_fail,conflict=conflict,policy=policy)
        for (type,stored,value),op,trigger,audit_fail,conflict,policy in itertools.product(profiles,
            ("set","null","delete"),(False,True),(False,True),(False,True),("none","allow","approved","deny"))]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--rust",required=True);p.add_argument("--cpp",required=True);p.add_argument("--check",action="store_true");a=p.parse_args()
    subprocess.run(["python3",str(ROOT/"scripts/verify_upstream.py")],check=True)
    cases=inputs(); rust=observe(a.rust,cases);cpp=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=y,cpp=z) for i,(x,y,z) in enumerate(zip(cases,rust,cpp)) if not same(y,z)]
    if gaps:
        target=ROOT/"docs/parity/dynamic_typed_save_audit_5_0_6_initial_gaps.json"
        if not target.exists(): target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+"\n")
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f"{len(gaps)}/{len(cases)} dynamic save/audit observations differ")
    source=json.loads((ROOT/"tests/generator/parity/dynamic_json_rust/PROVENANCE.json").read_text())
    corpus=dict(baseline=json.loads((ROOT/"upstream.lock.json").read_text())["commit"],producer=source,note="Actual unchanged same-model generated Product Q and audited save. SQLite storage/version, complete safe audit, exact ordered SQL/parameter/intent projections and ledger clearing. All ten supported dynamic datatypes, native typed seeding, null/typed-null and customer policy plan/approval/denial. Clock/timing fields are omitted symmetrically as nondeterministic observations; no output normalization.",cases=[dict(input=x,expected=y) for x,y in zip(cases,rust)])
    target=Path(__file__).with_name("dynamic_typed_save_audit_5_0_6_cases.json.gz")
    if a.check:
        with gzip.open(target,"rt") as f: assert same(corpus,json.load(f)),"Evidence differs from live source"
    else: target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+"\n").encode(),mtime=0))
    print(f"Compared {len(cases)} actual source/native durable dynamic save/audit observations")
if __name__=="__main__":main()
