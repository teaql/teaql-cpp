#!/usr/bin/env python3
"""Observe unchanged locked RuntimeDataService query methods."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    cases=[dict(input=dict(zip(['mode','capture','debug','failure','bad_model','event','trace'],values))) for values in itertools.product(['all','list','entities','enhanced'],*[ [False,True] for _ in range(6)])]
    for c,o in zip(cases,observe(args.rust,[c['input'] for c in cases])):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('runtime_base_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,separators=(',',':')) for c in cases)+'\n]}\n')
    print(f'Observed {len(cases)} locked runtime query adapter cases')
