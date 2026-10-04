#!/usr/bin/env python3
"""Observe unchanged pinned relation installation and recursive methods."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
def v(kind,value=None):return dict(kind=kind,**({} if value is None else dict(value=value)))
def u(n):return v('u64',str(n))
def s(value):return v('text',value)
def row(i,**kw):return dict(id=u(i),**kw)
def plan(name='children',target='Child',local='id',foreign='parent_id',many=True,**kw):return dict(name=name,target=target,local=local,foreign=foreign,many=many,**kw)
def inverse(entity='Child',name='back',target='Parent',local='parent_id',foreign='id',many=False):return dict(entity=entity,**plan(name,target,local,foreign,many))
def cases():
    out=[]
    def add(label,**kw):
        item=dict(mode='flat',parents=[row(1,label=s('p1')),row(2,label=s('p2'))],plans=[plan()],responses=[[row(10,parent_id=u(1),label=s('c10'),__teaql_partition_rank=v('i64','1')),row(11,parent_id=u(2),label=s('c11'),__teaql_partition_rank=v('i64','1'))]],metadata=[],register=['Parent','Child','Leaf'],pending=0,polls=20,fail_at=0,probes=False)
        item.update(kw);out.append(dict(label=label,input=copy.deepcopy(item)))
    modes=['attach','install','embedded','flat','compact']
    for mode,many,local,back in itertools.product(modes,[False,True],['id','ref'],['none','one','many','first']):
        foreign='parent_id' if local=='id' else 'id'
        metadata=[] if back=='none' else [inverse(local=foreign,foreign=local,many=back=='many')]
        if back=='first':metadata=[inverse(name='wrong',local='other')]+metadata+[inverse(name='ignored_second',local=foreign,foreign=local,many=True)]
        parents=[row(1,ref=u(10),children=s('old value')),row(2,ref=u(11))]
        children=[row(10,parent_id=u(1)),row(11,parent_id=u(2))]
        add('relation shape',mode=mode,plans=[plan(local=local,foreign=foreign,many=many)],parents=parents,responses=[children],metadata=metadata)
    for mode,variant in itertools.product(modes,range(13)):
        parents=[row(1),row(2)];children=[row(10,parent_id=u(1)),row(11,parent_id=u(2))]
        if variant==0:parents=[]
        if variant==1:children=[]
        if variant==2:parents=[row(1),row(1)]
        if variant==3:parents=[{},row(1)]
        if variant==4:parents=[dict(id=v('null'))];children=[row(10,parent_id=v('null'))]
        if variant==5:children=[dict(parent_id=u(1)),row(11,parent_id=u(2))]
        if variant==6:children=[dict(id=s('bad'),parent_id=u(1))]
        if variant==7:children=[row(10),row(11,parent_id=u(2))]
        if variant==8:children=[row(10,parent_id=u(1),fail=v('bool',True)),row(11,parent_id=u(2))]
        if variant==9:parents=[row(1,fail=v('bool',True)),row(2)]
        if variant==10:children=[row(10,parent_id=u(1),label=s('first')),row(10,parent_id=u(1),label=s('last'))]
        if variant==11:children=[row(20,parent_id=u(7))]
        if variant==12:parents=[dict(id=v('i64','1'))];children=[row(10,parent_id=u(1))]
        add('row/error boundary '+str(variant),mode=mode,parents=parents,responses=[children],metadata=[inverse()])
    for mode,previous in itertools.product(['attach','embedded','flat'],[v('null'),v('list',[s('old')]),v('object',dict(id=u(7))),s('unchanged')]):
        add('inverse existing payload',mode=mode,responses=[[row(10,parent_id=u(1),back=previous)]],metadata=[inverse(many=True)])
    for mode,registered in itertools.product(['install','flat','compact'],[[],['Parent'],['Child'],['Parent','Child','Leaf']]):
        add('decoder presence',mode=mode,register=registered,metadata=[inverse()])
    identities=[v('null'),dict(kind='typed_null',type='Text'),u(1),v('i64','1'),v('i64','-1'),s('1'),v('bool',False),v('f64','-0'),v('f64','1e-10'),v('decimal','1.00'),v('date','2024-02-29'),v('timestamp','123'),v('json',{'k':[1]}),v('list',[s('x')]),v('object',{'x':u(1)})]
    for mode,key in itertools.product(['attach','flat','compact'],identities):
        add('bucket key',mode=mode,plans=[plan(local='join',foreign='join')],parents=[row(1,join=key)],responses=[[row(10,join=key)]])
    for mode in modes:
        add('forward shared target',mode=mode,plans=[plan(local='ref',foreign='id',many=False)],parents=[row(1,ref=u(10)),row(2,ref=u(10)),row(3,ref=u(11))],responses=[[row(10),row(20)]],metadata=[inverse(local='id',foreign='ref')])
        add('owner missing ID after inverse',mode=mode,plans=[plan(local='ref',foreign='id',many=True)],parents=[dict(ref=u(10))],responses=[[row(10)]],metadata=[inverse(local='id',foreign='ref')])
    for mode,fail_at,pending,polls in itertools.product(['embedded','flat','compact'],[0,1,2,3,4],[0,1],[0,1,3,10]):
        child_plans=[plan('leafs','Leaf','id','child_id'),plan('reference','Leaf','leaf_ref','id',False)]
        children=[row(10,parent_id=u(1),leaf_ref=u(20),__teaql_partition_rank=v('i64','1')),row(11,parent_id=u(2),leaf_ref=u(21),__teaql_partition_rank=v('i64','1'))]
        first=row(20,child_id=u(10));second=row(21,child_id=u(11))
        responses=[children,[first],[first],[second],[second]] if mode=='embedded' else [children,[first,second],[first,second]]
        add('recursive order/failure',mode=mode,plans=[plan(children=child_plans,query=dict(entity='Child',relations=['leafs']))],responses=responses,metadata=[inverse(),inverse(entity='Leaf',name='owner',target='Child',local='child_id',foreign='id')],fail_at=fail_at,pending=pending,polls=polls)
    for mode,threshold in itertools.product(['embedded','flat','compact'],[0,1,3]):
        add('per-parent probes',mode=mode,plans=[plan(query=dict(entity='Child',slice=dict(offset=0,limit=1),threshold=threshold))],responses=[[row(10,parent_id=u(1))],[row(11,parent_id=u(2))]])
    for mode in ['embedded','flat','compact']:
        add('multiple root plans',mode=mode,plans=[plan(),plan('other')],responses=[[row(10,parent_id=u(1))],[row(11,parent_id=u(2))]])
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    data=cases()
    for c,o in zip(data,observe(args.rust,[c['input'] for c in data])):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('relation_hydration_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} locked relation hydration cases')
