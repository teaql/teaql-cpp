#!/usr/bin/env python3
"""Compare operation-owned leases, pending work and drop rollback."""
import argparse
import itertools
import json
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True)
    args=parser.parse_args()
    entries=[dict(count=count,end=end,parent=parent,write=write,polls=polls)
        for count,end,parent,write,polls in itertools.product((0,1,3),('commit','rollback','drop'),
        ('read','write','cancel','begin'),(False,True),(1,3))]
    original=observe(args.rust,entries);cpp=observe(args.cpp,entries)
    errors=[(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
    for i,entry,a,b in errors[:3]:print(f'Mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(entries)} lease observations differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],
        note='Actual operation-owned Rust transaction futures, held connection lease and released waiter; cancellation drops Rust future, C++ cancels operation; Rust transaction drop maps to actual C++ transaction UserContext destruction. Full states/results/rows, no normalization. Wake measures release notification independently from C++ cancellation notification.',
        cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    Path(__file__).with_name('sqlite_async_lease_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Compared {len(entries)} actual original/C++ asynchronous lease observations')


if __name__=='__main__':main()
