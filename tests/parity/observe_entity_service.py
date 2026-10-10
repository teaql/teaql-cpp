#!/usr/bin/env python3
"""Observe locked public EntityDataService and private relation planning functions."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe

def v(kind,value=None):return dict(kind=kind,**({} if value is None else {'value':value}))
def rel(name,target,local='id',foreign='owner',many=True):return dict(name=name,target=target,local=local,foreign=foreign,many=many)
def schemas():
    return [dict(name='Root',properties=[dict(name='id',type=2,id=True)],relations=[rel('children','Child'),rel('ref','Ref','ref','id',False)]),dict(name='Child',properties=[dict(name='child_id',type=2,id=True)],relations=[rel('owner','Root','owner','id',False),rel('ref','Ref','ref','id',False)]),dict(name='Ref',properties=[dict(name='id',type=2,id=True)],relations=[])]
def cases():
    out=[]
    def add(label,**kw):out.append(dict(label=label,input=dict(entities=schemas(),loads=['children'],rows=[],**kw)))
    def make(label,changes):
        j=dict(entities=schemas(),loads=['children'],rows=[]);j.update(changes);out.append(dict(label=label,input=j))
    paths=[[],['children'],['ref'],['children','ref'],['ref','children','children'],['children.owner.children','children.ref'],['children.'],['.children'],['unknown','children.bad'],['children.unknown','aaa'],['children.owner.children.owner'],['children..owner']]
    for paths in paths:make('behavior paths '+str(paths),dict(loads=paths))
    for missing in ['Root','Child','Ref']:
        for loads in [[],['children'],['ref'],['children.owner']]:make('missing '+missing+str(loads),dict(entities=[e for e in schemas() if e['name']!=missing],loads=loads))
    for name in ['children','ref','missing','children.owner']:make('public lookup '+name,dict(loads=['children','children.owner','ref'],find=name))
    identifiers=[v('null'),dict(kind='typed_null',type='I64'),v('u64','1'),v('i64','1'),v('i64','-1'),v('u64','2'),v('text','1'),v('text','long unicode身份'),v('bool',False),v('f64','-0'),v('f64','0'),v('f64','1e-10'),v('decimal','1.00'),v('json',{'k':[1]}),v('date','2024-02-29'),v('timestamp','123'),v('object',{'a':v('bool',True)}),v('list',[v('text','value')])]
    for i,identifier in enumerate(identifiers):
        make('bucket '+str(identifier),dict(rows=[{'id':identifier},{'id':identifiers[(i+1)%len(identifiers)]},{'id':identifier},{}]))
    for count in [0,1,2,19,20,21,32,65]:make('identity set '+str(count),dict(rows=[{'id':v('u64',str(i))} for i in reversed(range(count))]+[{'id':v('u64','2')}] if count else []))
    for projection,slice_,order in itertools.product([[],['name'],['owner','name']], [None,dict(offset=0,limit=2),dict(offset=1),dict(offset=3,limit=0)], [[],[dict(field='child_id',direction=1)],[dict(field='name',direction=0)],[dict(field='child_id',direction=1,expr=dict(kind='column',name='child_id'))]]):
        q=dict(entity='IgnoredTarget',projection=projection,order=order,relations=[dict(name='ref')],threshold=4,comment='nested query comment')
        if slice_ is not None:q['slice']=slice_
        q['filter']=dict(kind='binary',op=0,left=dict(kind='column',name='active'),right=dict(kind='value',value=v('bool',True)))
        make('explicit query modifiers',dict(query=dict(entity='Root',relations=[dict(name='children',query=q)]),rows=[{'id':v('u64','2')},{'id':v('i64','1')},{'id':v('u64','2')}]))
    for names in [['ref','children','ref'],['children','children'],['missing','children'],['children','missing'],[]]:make('explicit order '+str(names),dict(query=dict(entity='Root',relations=[dict(name=name) for name in names])))
    for missing in ['Root','Child','Ref']:
        make('explicit missing '+missing,dict(entities=[e for e in schemas() if e['name']!=missing],query=dict(entity='Root',relations=[dict(name='children',query=dict(relations=[dict(name='ref')]))])))
    for properties in [[],[dict(name='custom_identity',type=2,id=True)]]:
        entities=schemas();entities[1]['properties']=properties
        make('target identity sort '+str(properties),dict(entities=entities,query=dict(relations=[dict(name='children',query=dict(slice=dict(offset=0,limit=3),partition='existing'))])))
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    data=cases();outputs=observe(args.rust,[c['input'] for c in data])
    for c,o in zip(data,outputs):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('entity_service_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,ensure_ascii=True,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} locked entity service planning cases')
