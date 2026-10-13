#!/usr/bin/env python3
"""Replay historical provider inputs against actual locked 5.0.6 calls."""
import argparse
import gzip
import json
from pathlib import Path
from compare_values import observe,same

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True)
    args=parser.parse_args()
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    for family in ('sqlite_transport','sqlite_id','sqlite_business_id'):
        historical=Path(__file__).with_name(family+'_cases.json')
        old=json.loads(historical.read_text())
        inputs=[case['input'] for case in old['cases']]
        original=observe(args.rust,inputs);cpp=observe(args.cpp,inputs)
        errors=[(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(inputs,original,cpp)) if not same(a,b)]
        for i,entry,a,b in errors[:3]:print(f'{family} mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
        if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} {family} observations differ')
        corpus=dict(baseline=lock['commit'],version=lock['version'],
            historical_baseline=old['baseline'],
            note='Historical input calls replayed against actual locked 5.0.6 provider. New Rust driver adapts the separate transaction type for compilation. This family does not execute that asynchronous transaction branch. Complete outputs, no normalization; historical artifacts intact.',
            cases=[dict(input=entry,expected=result) for entry,result in zip(inputs,original)])
        target=historical.with_name(family+'_5_0_6_cases.json.gz')
        target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
        print(f'Compared {len(inputs)} actual {family} original/C++ observations',flush=True)


if __name__=='__main__':main()
