#!/usr/bin/env python3
"""Replay the complete historical synchronous transaction family against 5.0.6."""
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
    old=json.loads(Path(__file__).with_name('sqlite_transaction_cases.json').read_text())
    entries=[case['input'] for case in old['cases'] if not case['input']['async']]
    original=observe(args.rust,entries);cpp=observe(args.cpp,entries)
    errors=[(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
    for i,entry,a,b in errors[:3]:print(f'Mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(entries)} synchronous transaction observations differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],historical_baseline=old['baseline'],
        note='All historical synchronous transaction inputs replayed against actual locked 5.0.6. Complete step/errors/rows/autocommit, no normalization. Original full 366-case family preserved; the 183 asynchronous inputs require protocol migration, not relabeling.',
        cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    target=Path(__file__).with_name('sqlite_sync_transaction_5_0_6_cases.json.gz')
    target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared all {len(entries)} original/C++ synchronous transaction observations')


if __name__=='__main__':main()
