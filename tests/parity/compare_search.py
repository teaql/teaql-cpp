#!/usr/bin/env python3
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe,same

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--cpp',required=True);parser.add_argument('--rust');args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    corpus=json.loads(Path(__file__).with_name('search_cases.json').read_text())
    if corpus['baseline']!=json.loads((root/'upstream.lock.json').read_text())['commit']:raise RuntimeError('Search corpus baseline differs')
    inputs=[dict(corpus['setup'],**case['input']) for case in corpus['cases']]
    expected=[case['expected'] for case in corpus['cases']]
    if args.rust:
        subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],cwd=root,check=True)
        if not same(expected,observe(args.rust,inputs)):raise RuntimeError('Committed search observations differ from live Rust')
    actual=observe(args.cpp,inputs)
    failures=[(i,inputs[i],a,b) for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    for i,source,a,b in failures[:12]:print(f'Case {i}: {source}\n Rust: {a}\n C++: {b}')
    print(f'Dynamic search parity: {len(inputs)-len(failures)}/{len(inputs)} observations match')
    return bool(failures)
if __name__=='__main__':raise SystemExit(main())
