#!/usr/bin/env python3
"""Record the actual locked driver's enabled scalar/text/unit alternatives."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    inputs = [{"kind":kind,"changed":{"kind":3 if kind != 3 else 4}}
              for kind in [0,3,4,8,9,10]]
    for index,value in itertools.product(["0","1","31","2147483647"],
                                         ["-9223372036854775808","-1","0","1","9223372036854775807"]):
        inputs.append({"kind":1,"index":index,"value":value,
                       "changed":{"kind":1,"index":index,"value":"1" if value != "1" else "0"}})
    for kind,name in itertools.product([2,5],["", "name", "quote\"\\\n\x00雪🧊", "long:" + "x"*256]):
        inputs.append({"kind":kind,"name":name,"changed":{"kind":kind,"name":name+"!"}})
    for kind,index in itertools.product([6,11],["0","1","31","4294967295"]):
        inputs.append({"kind":kind,"index":index,
                       "changed":{"kind":kind,"index":"1" if index != "1" else "0"}})
    for index,kind in itertools.product(["0","1","31","4294967295"],range(5)):
        inputs.append({"kind":7,"index":index,"type":kind,
                       "changed":{"kind":7,"index":index,"type":(kind+1)%5}})
    expected = observe(args.rust, inputs)
    corpus = {"baseline":json.loads((root/"upstream.lock.json").read_text())["commit"],
              "note":"Actual enabled rusqlite alternatives; Debug/Display/active equality/cause. Driver facade and boxed conversions require separate integration.",
              "cases":[{"input":value,"expected":result} for value,result in zip(inputs,expected)]}
    target = Path(__file__).with_name("sqlite_simple_error_cases.json.gz")
    with target.open("wb") as out:
        with gzip.GzipFile(fileobj=out,mode="wb",mtime=0) as compressed:
            compressed.write((json.dumps(corpus,ensure_ascii=True,separators=(",", ":"))+"\n").encode())
    print(f"Recorded {len(inputs)} actual scalar/text/unit error observations")


if __name__ == "__main__":
    main()
