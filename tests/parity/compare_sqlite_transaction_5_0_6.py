#!/usr/bin/env python3
"""Verify every historical transaction input through the qualified 5.0.6 routes."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp',required=True)
    parser.add_argument('--cpp-async',required=True)
    parser.add_argument('--cpp-emulator',default='')
    parser.add_argument('--rust')
    parser.add_argument('--rust-async')
    args=parser.parse_args()
    original=json.loads(Path(__file__).with_name('sqlite_transaction_cases.json').read_text())
    import gzip
    routes=[('sqlite_sync_transaction_5_0_6',args.cpp,args.rust,False),
            ('sqlite_async_schedule_5_0_6',args.cpp_async,args.rust_async,True)]
    for family,cpp,rust,asynchronous in routes:
        with gzip.open(Path(__file__).with_name(family+'_cases.json.gz'),'rt') as source:
            corpus=json.load(source)
        historical=[case['input'] for case in original['cases'] if case['input']['async']==asynchronous]
        if [case['input'] for case in corpus['cases']]!=historical:
            raise RuntimeError('Versioned transaction evidence omits or changes historical inputs')
        command=[sys.executable,str(Path(__file__).with_name('compare_'+family+'.py')),
                 '--cpp',cpp,'--cpp-emulator',args.cpp_emulator]
        if rust:command+=['--rust',rust]
        subprocess.run(command,check=True,cwd=ROOT)
    print(f'Qualified all {len(original["cases"])} historical transaction inputs through versioned 5.0.6 routes')


if __name__=='__main__':main()
