#!/usr/bin/env python3
"""Compare original cached readonly schema hashes and full query/row diagnostics."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def inputs():
    cases = [dict(fields=[])]
    names = ["_a", "_雪😀", "_a\u0000b", "_" + "x" * 4096]
    for name in names:
        for kind in range(10):
            cases.append(dict(fields=[[name, kind]]))
    for count in (2, 8, 32, 100):
        fields = [[f"_field_{index}", index % 10] for index in range(count)]
        cases.extend((dict(fields=fields), dict(fields=list(reversed(fields)))))
    for kind in range(10):
        fields = [["_first", kind], ["_second", (kind+1) % 10]]
        cases.extend((dict(fields=fields), dict(fields=list(reversed(fields)))))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    cases = inputs(); original = observe(args.rust, cases); native = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(original, native)) if not same(a, b)]
    for i in failures[:3]:
        print(f"Case {i}: {cases[i]}\nRust: {original[i]}\nC++: {native[i]}")
    if failures:
        raise SystemExit(f"{len(failures)}/{len(cases)} readonly schema observations differ")
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"],
        note="Actual original cached u64 Hash trait and complete readonly schema/query/uninitialized "
             "row/layout diagnostics. Exact source output, no numeric or text normalization.",
        cases=[dict(input=entry, expected=result) for entry, result in zip(cases, original)])
    target = Path(__file__).with_name("readonly_schema_hash_5_0_6_cases.json.gz")
    if args.check:
        with gzip.open(target, "rt", encoding="utf-8") as source:
            if not same(corpus, json.load(source)):
                raise SystemExit("Committed readonly schema evidence differs from live source")
    else:
        target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    print(f"Compared {len(cases)} exact original/native readonly schema observations")


if __name__ == "__main__":
    main()
