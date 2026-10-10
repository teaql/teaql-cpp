#!/usr/bin/env python3
"""Record original SafeExpression calls, TeaqlEmpty and callback order."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    def value(kind, data=None):
        return dict(kind=kind, value=data)
    pool = [value("null"),value("typed_null"),value("bool",False),value("bool",True),
            value("i64","-9223372036854775808"),value("u64","18446744073709551615"),
            value("f64","NaN"),value("f64","-0.0"),value("decimal","1.20"),
            value("text",""),value("text","雪\n\0"),value("date","2024-02-29"),
            value("timestamp","42"),value("json",None),value("json",""),
            value("json",[]),value("json",{}),value("json",{"k":[1,None]}),
            value("list",[]),value("list",[value("null")]),value("object",{}),
            value("object",{"x":value("list",[value("text","nested")])})]
    cases=[]
    for index,root in enumerate(pool):
        for present in [False,True]:
            for mode in ["identity","missing","wrap"]:
                for size in [0,1,3]:
                    cases.append(dict(root=root,alternate=pool[(index+5)%len(pool)],
                                      fallback=pool[(index+11)%len(pool)],present=present,mode=mode,
                                      list=[root]*size,index=[0,1,4294967295][size%3]))
    for root in ["", "root", "雪\n\0", "x"*4096]:
        for capture in ["", "-capture", "雪"]:
            cases.append(dict(kind="owned",root=root,capture=capture))
    for identifier in ["0","1","18446744073709551615"]:
        for version in ["-9223372036854775808","0","1","9223372036854775807"]:
            for next_id in ["0","7","18446744073709551615"]:
                cases.append(dict(kind="entity",id=identifier,version=version,next=next_id))
    for present in [False, True]:
        for empty in [False, True]:
            cases.append(dict(kind="custom_empty", present=present, empty=empty))
    return cases


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--rust",required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(["python3",str(root/"scripts/verify_upstream.py")],check=True)
    cases=inputs()
    outputs=observe(args.rust,cases)
    lock=json.loads((root/"upstream.lock.json").read_text())
    corpus=dict(baseline=lock["commit"],note="Original SafeExpression public calls and exact callback trace, Value emptiness, identity roots and list adapters.",
                cases=[dict(input=x,expected=y) for x,y in zip(cases,outputs)])
    Path(__file__).with_name("safe_expression_api_cases.json").write_text(json.dumps(corpus,indent=2)+"\n")
    print(f"Recorded {len(cases)} original SafeExpression API inputs")


if __name__=="__main__":
    main()
