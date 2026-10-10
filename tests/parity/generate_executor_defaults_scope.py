#!/usr/bin/env python3
"""Record public pinned trait-default delegation without copying default bodies."""
import argparse
import itertools
import json
import subprocess
from pathlib import Path
from compare_values import observe

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    cases = [dict(name=n,flags=f,observer=o,poll=p,chunk_size=c) for n,f,o,p,c in itertools.product(
        ['Device','边缘设备','D'*256],range(4),[False,True],[False,True],[0,1,17])]
    outputs = observe(args.rust,cases)
    corpus = dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
        note='Calls original inherited observed defaults; fixture implements mandatory SPIs only.',
        cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    Path(__file__).with_name('executor_defaults_scope_cases.json').write_text(json.dumps(corpus,separators=(',',':'))+'\n')
    print(f'Recorded {len(cases)} original executor-default inputs')

if __name__ == '__main__':
    main()
