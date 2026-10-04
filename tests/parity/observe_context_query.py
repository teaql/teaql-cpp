#!/usr/bin/env python3
"""Observe unchanged pinned ContextDataService query/telemetry methods."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe

def cases():
    result=[]
    def add(**kw):
        item=dict(mode='all',noop=False,start_fail=False,finish_fail=False,failure=False,bad_model=False,event=True,pending=0,polls=3,activation='normal',capture=True,user='user',comment='business',trace=True)
        item.update(kw);result.append(dict(input=item))
    for mode,capture,flow,outcome,noop in itertools.product(['all','list','entities','enhanced'],[False,True],['ready','pending','cancel','unpolled'],['success','executor','model'],[False,True]):
        add(mode=mode,capture=capture,noop=noop,pending=2 if flow in ['pending','cancel'] else 0,polls=0 if flow=='unpolled' else 1 if flow=='cancel' else 3,failure=outcome=='executor',bad_model=outcome=='model')
    for mode,user,comment,trace in itertools.product(['all','list','entities','enhanced'],[None,'',' user ','用户'],[None,'','business'],[False,True]):
        add(mode=mode,user=user,comment=comment,trace=trace)
    for mode,activation,flow,fault in itertools.product(['all','list','entities','enhanced'],['before','skip','after','twice','swallow'],['ready','pending','cancel'],['none','start','finish']):
        add(mode=mode,activation=activation,pending=2 if flow!='ready' else 0,polls=1 if flow=='cancel' else 3,start_fail=fault=='start',finish_fail=fault=='finish')
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    data=cases()
    for c,o in zip(data,observe(args.rust,[c['input'] for c in data])):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('context_query_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} locked Context query cases')
