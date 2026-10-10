#!/usr/bin/env python3
"""Observe the complete locked entity-runtime module, including private invariants."""
import argparse
import copy
import json
from pathlib import Path
import random
import subprocess
from compare_values import observe


def v(kind, value=None):
    return dict(kind=kind, **({} if value is None else {'value': value}))


def key(entity='Item', identifier=None):
    return dict(entity=entity, id=identifier or v('u64', '7'))


def step(op, state=0, **kw):
    return dict(op=op, state=state, **kw)


def cases():
    cases=[]
    def ledger(label, steps, keys=None):
        cases.append(dict(label=label,input=dict(mode='ledger',keys=keys or [key(),key(identifier=v('i64','7')),key('Other')],steps=steps)))
    ledger('pristine reads and clear', [step(x) for x in ['pop','clear','commit','compose','adopt']])
    ledger('shared lazy state and independent fresh', [step('clone',1,other=0),step('set',1,field='a',value=v('text','shared')),step('fresh',2,other=0),step('set',2,field='b',value=v('bool',True)),step('share',2,other=1),step('comment',1,value='')])
    ledger('stack shadow, pop, delete across levels',[step('set',field='a',value=v('text','base')),step('push'),step('set',field='a',value=v('text','top')),step('set',field='b',value=v('bool',True)),step('pop'),step('push'),step('new'),step('delete'),step('set',field='name',value=v('text','after delete')),step('commit')])
    for loaded_id in [v('u64','7'),v('i64','7'),v('text','7'),v('text','bad'),v('i64','-1'),v('null')]:
        for version in [v('i64','2'),v('text','3'),v('text','bad'),v('null')]:
            ledger('loaded snapshot ID/version',[step('load',entity='Item',values={'id':loaded_id,'version':version,'name':v('text','original')}),step('clone',1,other=0),step('snapshot',values={'name':v('text','ledger snapshot')}),step('version',value='9'),step('commit')])
    for target_version in [None,2,3]:
        for source_version in [None,2,3]:
            for loaded in [False,True]:
                steps=[]
                for state,version in [(0,target_version),(1,source_version)]:
                    if version is not None:steps.append(step('load',state,entity='Item',values={'id':v('i64','7'),'version':v('i64',str(version))}) if loaded else step('version',state,value=str(version)))
                steps += [step('set',1,field='name',value=v('text','source')),step('new',1),step('comment',1,value='source reason'),step('compose',other=1),step('adopt',other=1),step('commit'),step('compose',other=1),step('set',1,field='name',value=v('text','later'))]
                ledger(f'atomic composition {target_version}/{source_version}/{loaded}',steps)
    ledger('version conflict traversal first then ordered overflow',[step('version',key=2,value='1'),step('version',key=0,value='1'),step('version',1,key=2,value='2'),step('version',1,key=0,value='3'),step('compose')])
    ledger('source changes from every stack merge into target current',[step('set',field='a',value=v('text','target')),step('push'),step('set',1,field='a',value=v('text','source lower')),step('push',1),step('set',1,field='a',value=v('text','source upper')),step('set',1,field='b',value=v('bool',True)),step('compose'),step('pop'),step('pop',1)])
    ledger('target empty comment wins, source snapshot retained',[step('comment',value=''),step('comment',1,value='source'),step('snapshot',1,values={'a':v('text','one')}),step('compose'),step('snapshot',1,values={'a':v('text','two')}),step('compose')])
    identity=[v('null'),dict(kind='typed_null',type='I64'),v('bool',False),v('i64','-1'),v('i64','7'),v('u64','7'),v('text','7'),v('f64','0'),v('f64','-0'),v('decimal','7.00'),v('decimal','7'),v('object',{}),v('object',{'a':v('bool',True)}),v('list',[]),v('list',[v('bool',True)]),v('json',{'a':1})]
    for i,identifier in enumerate(identity):
        ledger(f'identity representations {i}',[step('set',field='a',value=v('text','first')),step('new'),step('set',key=1,field='b',value=v('bool',True)),step('version',value='2'),step('version',1,key=1,value='3'),step('compose'),step('delete',key=1)],keys=[key(identifier=identifier),key(identifier=identity[(i+1)%len(identity)])])
    # Captures the source's optional-ID comparison in composition, which differs
    # intentionally from get_original_version's Some guard.
    ledger('nonnumeric loaded ID composition',[step('load',entity='Item',values={'id':v('text','bad'),'version':v('i64','2')}),step('version',1,value='3'),step('compose')],keys=[key(identifier=v('text','different'))])
    rng=random.Random(0x5051ED6E)
    operations=['set','set','push','pop','clear','commit','new','delete','version','comment','clone','fresh','share','weak','reset','adopt','compose','load','snapshot']
    for seed in range(128):
        steps=[]
        for i in range(18):
            op=rng.choice(operations);s=step(op,rng.randrange(3),other=rng.randrange(3),key=rng.randrange(3))
            if op=='set':s.update(field=rng.choice(['a','b','name']),value=rng.choice([v('text','value-'+str(i)),v('bool',False),v('null'),v('object',{'name':v('text','nested')})]))
            if op=='version':s['value']=str(rng.randrange(-3,5))
            if op=='comment':s['value']=rng.choice(['','first','后来'])
            if op in ['load','snapshot']:s.update(entity=rng.choice(['Item','Other']),values={'id':v('i64',str(rng.choice([7,8,-1]))),'version':v('i64',str(rng.randrange(1,5))),'name':v('text','stored')})
            steps.append(s)
        ledger(f'seed 0x5051ED6E / {seed}',steps)
    for weak in [False,True]:
        for cloned in [False,True]:
            for owner in ['PurchaseOrder','purchase_order','URL','ΣRoot']:
                for mode in ['list','option','text']:
                    canonical=''.join(('_' if i else '')+c.lower() if 'A'<=c<='Z' else c for i,c in enumerate(owner))
                    entries=[dict(id=1,type='number',value='7'),dict(id=1,type='text',value='first'),dict(id=1,type='number',value='9')]
                    relations=[dict(owner=owner,id=1,name='filled',type=mode,values=[7,8],value=9 if mode!='text' else 'text'),dict(owner=owner,id=1,name='empty',type=mode,values=[],value=None)]
                    queries=[dict(owner=o,id=i,name=n,type=t) for o,i,n in [(owner,1,'filled'),(canonical,1,'filled'),(owner,1,'empty'),(owner,1,'missing'),(owner,2,'filled')] for t in ['list','option']]
                    cases.append(dict(label=f'graph {weak}/{cloned}/{owner}/{mode}',input=dict(mode='graph',weak=weak,clone=cloned,entries=entries,relations=relations,lookups=[1,2],queries=queries)))
    return cases

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    data=cases();outputs=observe(args.rust,[c['input'] for c in data])
    for c,o in zip(data,outputs):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('entity_runtime_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,ensure_ascii=True,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} locked entity-runtime cases')
