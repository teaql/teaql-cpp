#!/usr/bin/env python3
"""Record direct public Rust 5.0.6 dynamic mutation/loaded-view state calls."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def load(values=None,executor=None):
    result=dict(owner='Item',values=values if values is not None else {'name':dict(kind='text',value='original')})
    if executor is not None:result['executor']=executor
    return result
def action(op,state=0,**kw):return dict(op=op,state=state,**kw)
def cases():
    out=[]
    for initial in [None,load({}),load(),load({'name':dict(kind='null')})]:
        for code in ['name','count','missing','_readonly','#name']:
            for operand in [dict(kind='null'),dict(kind='typed_null'),dict(kind='bool',value=False),dict(kind='i64',value='0'),dict(kind='text',value='next'),dict(kind='json',value={})]:
                out.append(dict(initial=[initial],actions=[action('set',code=code,value=operand),action('delete',code=code),action('clear'),action('pop')]))
    for initial in [None,load({}),load()]:
        for actions in [
            [action('clone'),action('set',state=1,code='name',value=dict(kind='text',value='child'))],
            [action('fresh'),action('rebind',state=1,**{'from':0}),action('set',state=1,code='name',value=dict(kind='text',value='child'))],
            [action('push'),action('set',code='name',value=dict(kind='null')),action('push'),action('delete',code='name'),action('pop'),action('clear'),action('commit')],
            [action('set',entity='Other',code='name',value=dict(kind='text',value='wrong'))],
            [action('set',code='name',value=dict(kind='text',value='before')),action('load',load=load({'name':dict(kind='text',value='reload')})),action('delete',code='name')],
        ]:out.append(dict(initial=[initial],actions=actions))
    for receiving in [load(),load(executor=1)]:
        for source in [load(),load(executor=1),load(executor=2)]:
            for receiver_changes in [False,True]:
                actions=[action('comment',text='receiving reason'),action('comment',state=1,text='source reason')]
                if receiver_changes:actions.append(action('set',code='name',value=dict(kind='text',value='receiving pending')))
                actions += [action('set',state=1,code='name',value=dict(kind='text',value='source pending')),action('native',state=1,value=dict(kind='i64',value='99')),action('adopt',**{'from':1}),action('adopt',**{'from':1}),action('commit')]
                out.append(dict(initial=[receiving,source],actions=actions))
    return out
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);args=parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    for i,(a,b) in enumerate(zip(expected,actual)):
        if not same(a,b):
            Path('/tmp/teaql_dynamic_ledger_mismatch.json').write_text(json.dumps(dict(index=i,input=inputs[i],rust=a,cpp=b),indent=2));raise SystemExit(f'Dynamic ledger mismatch {i}; see /tmp/teaql_dynamic_ledger_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());sources=['teaql-runtime/src/entity_runtime.rs','teaql-core/src/dynamic_fields.rs']
    corpus=dict(baseline=lock['commit'],version=lock['version'],source=sources,source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest() for s in sources},
        note='Direct public original state loading, dynamic mutation, clones, frames, composition and view equality. Private originals/provenance maps, protected SQL, generated carriers and full Debug layouts are not qualified by these observations.',cases=[dict(input=i,expected=o)for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_ledger_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n');print(f'Compared and recorded {len(inputs)} public dynamic ledger observations')
if __name__=='__main__':main()
