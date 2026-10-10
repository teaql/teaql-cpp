#!/usr/bin/env python3
"""Record original telemetry conversions, equality, Clone and Debug directly."""
import argparse
import gzip
import itertools
import json
import subprocess
from pathlib import Path
from compare_values import observe

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--rust',required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    values=[dict(kind='string',value=s) for s in ['', 'Label\n"quoted"','边缘设备','x'*512]]
    values += [dict(kind='i64',value=str(v)) for v in [-(1<<63),-1,0,(1<<63)-1]]
    values += [dict(kind='usize',value=str(v)) for v in [0,1,(1<<32)-1]]
    values += [dict(kind='bool',value=v) for v in [False,True]]
    values += [dict(kind='float',value=v) for v in ['0000000000000000','8000000000000000','0000000000000001',
        '3ff4000000000000','7fefffffffffffff','7ff0000000000000','fff0000000000000','7ff8000000000001','fff8000000000001']]
    keys=['custom','teaql.operation.family','teaql.operation.name','teaql.entity.id','http.request.body']
    cases=[dict(**v,key=k,family='query',name='Device.run') for v,k in itertools.product(values,keys)]
    outputs=observe(args.rust,cases)
    corpus=dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
        note='Original conversions/PartialEq/Clone/Debug; usize inputs fit both native widths.',
        cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name('telemetry_model_scope_cases.json.gz')),mode='wb',mtime=0) as f:
        f.write((json.dumps(corpus,separators=(',',':'))+'\n').encode())
    print(f'Recorded {len(cases)} original telemetry model inputs')

if __name__ == '__main__':
    main()
