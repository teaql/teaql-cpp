#!/usr/bin/env python3
"""Compare native-width raw converted Debug; no sentinel text normalization."""
import argparse
import gzip
import json
from pathlib import Path
from compare_values import observe, same


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpp",required=True)
    parser.add_argument("--rust")
    args=parser.parse_args()
    with gzip.open(Path(__file__).with_name("sqlite_from_sql_raw_debug_5_0_6_cases.json.gz"),"rt") as source:
        corpus=json.load(source)
    root=Path(__file__).resolve().parents[2]
    assert corpus["baseline"]==json.loads((root/"upstream.lock.json").read_text())["commit"]
    inputs=[case["input"] for case in corpus["cases"]]
    expected=[case["expected"] for case in corpus["cases"]]
    if args.rust:
        live=observe(args.rust,inputs,arguments=["--raw-debug"])
        if not same(live,expected):raise RuntimeError("Raw converted observations differ from actual original calls")
    actual=observe(args.cpp,inputs,arguments=["--raw-debug"])
    mismatches=[index for index,(first,second) in enumerate(zip(expected,actual)) if not same(first,second)]
    for index in mismatches[:3]:print(index,expected[index],actual[index])
    print(f"Native raw converted diagnostics: {len(inputs)-len(mismatches)}/{len(inputs)} match")
    return bool(mismatches)


if __name__=="__main__":raise SystemExit(main())
