#!/usr/bin/env python3
"""Replay all historical async transaction inputs with a shared explicit scheduler."""
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
    entries=[case['input'] for case in old['cases'] if case['input']['async']]
    original=observe(args.rust,entries);cpp=observe(args.cpp,entries)
    errors=[(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
    for i,entry,a,b in errors[:3]:print(f'Mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(entries)} scheduled transaction observations differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],historical_baseline=old['baseline'],
        note='All 183 historical async transaction input actions remain unchanged. Actual Rust/C++ calls use an explicit scheduling adapter: poll on submission, one ordered pending sweep after each input action, cancel outstanding waiters at EOF then drop active transaction, observe final rows/autocommit. Steps/resumes/cancellations/errors are unnormalized. Historical ready-only driver/corpus remains intact.',
        cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    target=Path(__file__).with_name('sqlite_async_schedule_5_0_6_cases.json.gz')
    target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared all {len(entries)} original/C++ asynchronous schedules')


if __name__=='__main__':main()
