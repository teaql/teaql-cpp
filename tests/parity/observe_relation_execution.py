#!/usr/bin/env python3
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe

def cases():
    result=[]
    def add(**kw):
        item=dict(probes=True,capture=True,compact=True,threshold=None,parents=3,many=True,query=True,slice='limit',pending=0,polls=10,fail_at=0)
        item.update(kw);result.append(dict(input=item))
    for probes,capture,compact,threshold,parents in itertools.product([False,True],[False,True],[False,True],[None,0,2,9],[0,1,2,3]):
        add(probes=probes,capture=capture,compact=compact,threshold=threshold,parents=parents)
    for probes,capture,many,slice_,threshold in itertools.product([False,True],[False,True],[False,True],['none','offset','zero'],[None,0,2,9]):
        add(probes=probes,capture=capture,many=many,slice=slice_,threshold=threshold)
    for query,probes,capture,compact in itertools.product([False,True],[False,True],[False,True],[False,True]):
        add(query=query,probes=probes,capture=capture,compact=compact,parents=21)
    for probes,capture,compact,fail_at,pending,polls in itertools.product([False,True],[False,True],[False,True],[0,1,2,3],[0,1],[0,1,4]):
        add(probes=probes,capture=capture,compact=compact,threshold=9,fail_at=fail_at,pending=pending,polls=polls)
    return result
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    data=cases()
    for c,o in zip(data,observe(args.rust,[c['input'] for c in data])):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('relation_execution_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} locked relation execution cases')
