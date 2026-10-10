#!/usr/bin/env python3
"""Record actual source registry derives without changing implementation headers."""
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
    subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    cases=[]
    for name,mode in itertools.product(['Item','设备','Long'+'x'*128],range(6)):
        entries=[dict(name='Zoo',table='zoo',fields=[]),
                 dict(name=name,table='old_table',fields=['id','label'])]
        if mode&1: entries=list(reversed(entries))
        if mode&2: entries.append(dict(name=name,table='duplicate_table',fields=['payload']))
        if mode==4: entries=[]
        if mode==5: entries.append(dict(name='Alpha',table='alpha',fields=['first','second']))
        cases.append(dict(entries=entries,names=[name,'OnlyName',name,''],snapshot=name,
            replacement=dict(name=name,table='replacement_table',fields=['replacement','value'])))
    outputs=observe(args.rust,cases)
    corpus=dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
        note='Actual original Default/Clone/Debug, ordering, duplicates, replacement and schema snapshot; native implementation pending.',
        cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name('registry_debug_scope_cases.json.gz')),mode='wb',mtime=0) as target:
        target.write((json.dumps(corpus,separators=(',',':'))+'\n').encode())
    print(f'Recorded {len(cases)} original registry diagnostic inputs')

if __name__ == '__main__':
    main()
