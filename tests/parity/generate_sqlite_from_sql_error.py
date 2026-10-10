#!/usr/bin/env python3
"""Actual FromSqlError calls and portable conversion responsibilities."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust",required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    inputs=[{"kind":"invalid"}]
    inputs.extend({"kind":"range","value":value} for value in
                  ["-9223372036854775808","-1","0","1","9223372036854775807"])
    inputs.extend({"kind":"blob","expected":expected,"actual":actual} for expected,actual
                  in itertools.product(["0","1","16","4294967295"],repeat=2))
    invalid=[b"\xff",b"\xc2",b"\xe1\x80",b"\xf0\x90\x80",b"\xed\xa0\x80",b"a\xffb","雪".encode()+b"\xff"]
    inputs.extend({"kind":kind,"bytes":list(value)} for kind,value
                  in itertools.product(["utf8","other"],invalid))
    expected=observe(args.rust,inputs)
    corpus={"baseline":json.loads((root/"upstream.lock.json").read_text())["commit"],
            "note":"Actual FromSqlError Debug/Display/equality/source and original driver conversion. Unknown index is observed semantically as usize::MAX, without normalizing diagnostic text; raw converted driver Debug has target-width-dependent index and requires separate native evidence.",
            "cases":[{"input":value,"expected":result} for value,result in zip(inputs,expected)]}
    target=Path(__file__).with_name("sqlite_from_sql_error_cases.json.gz")
    with target.open("wb") as out:
        with gzip.GzipFile(fileobj=out,mode="wb",mtime=0) as compressed:
            compressed.write((json.dumps(corpus,ensure_ascii=True,separators=(",", ":"))+"\n").encode())
    raw_expected=observe(args.rust,inputs,arguments=["--raw-debug"])
    raw_corpus={"baseline":corpus["baseline"],
                "note":"Actual native 64-bit Rust converted diagnostics, including the unmodified usize::MAX text in all formatter modes. Run this profile only against native 64-bit C++.",
                "cases":[{"input":value,"expected":result} for value,result in zip(inputs,raw_expected)]}
    sentinel=str((1 << 64)-1)
    if not all(sentinel in result["converted_raw_debug"]["compact"] for result in raw_expected):
        raise RuntimeError("Raw diagnostics require a 64-bit original oracle")
    raw_target=Path(__file__).with_name("sqlite_from_sql_raw_debug_cases.json.gz")
    with raw_target.open("wb") as out:
        with gzip.GzipFile(fileobj=out,mode="wb",mtime=0) as compressed:
            compressed.write((json.dumps(raw_corpus,ensure_ascii=True,separators=(",", ":"))+"\n").encode())
    print(f"Recorded {len(inputs)} portable and native raw column-conversion observations")


if __name__=="__main__":
    main()
