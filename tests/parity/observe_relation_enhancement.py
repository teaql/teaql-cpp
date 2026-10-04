#!/usr/bin/env python3
"""Observe pinned aggregate, object-group and child-query enhancement methods."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
def v(kind,value=None):return dict(kind=kind,**({} if value is None else dict(value=value)))
def u(n):return v('u64',str(n))
def t(s):return v('text',s)
def row(i,**kw):return dict(id=u(i),**kw)
def cache(enabled=True,propagate=False,ttl=17,child_ttl=29):return dict(enabled=enabled,propagate=propagate,ttl=ttl,child_ttl=child_ttl)
def entities():return [dict(name='Parent',properties=[dict(name='id',type=2,id=True)],relations=[dict(name='children',target='Child',local='id',foreign='parent_id')]),dict(name='Child',properties=[dict(name='id',type=2,id=True),dict(name='parent_id',type=2,column='parent_col')],relations=[dict(name='leafs',target='Leaf',local='id',foreign='child_id')]),dict(name='Leaf',properties=[])]
def enhancement(mode,**kw):
    out=dict(query=dict(entity='Child'))
    if mode=='aggregate':out.update(relation='children',alias='stats',single=True)
    if mode=='group':out.update(property='object',storage='ref')
    out.update(kw);return out
def cases():
    data=[]
    def add(label,mode,**kw):
        responses=[[dict(parent_id=u(1),count=u(3)),dict(parent_id=u(2),count=u(4))]] if mode=='aggregate' else [[row(1,label=t('first')),row(2,label=t('second'))]]
        item=dict(mode=mode,entities=entities(),parents=[row(1,ref=u(1),label=t('parent')),row(2,ref=u(2))],enhancements=[enhancement(mode)],responses=responses,pending=0,polls=8,fail_at=0,cache=None);item.update(kw);data.append(dict(label=label,input=copy.deepcopy(item)))
    for single,variant in itertools.product([False,True],range(14)):
        children=[dict(parent_id=u(1),count=u(3)),dict(parent_id=u(1),count=u(4)),dict(parent_id=u(2),count=u(5))];parents=[row(1),row(2),row(3),{}]
        if variant==0:children=[]
        if variant==1:children=[dict(parent_id=u(1))]
        if variant==2:children=[dict(parent_id=u(1),a=u(2),b=t('other'))]
        if variant==3:children=[dict(parent_col=u(1),count=u(9))]
        if variant==4:children=[dict(parent_id=u(1),parent_col=u(2),count=u(9))]
        if variant==5:children=[dict(count=u(9))]
        if variant==6:parents=[]
        if variant==7:parents=[{},dict(stats=t('previous'))]
        if variant==8:parents=[row(1),row(1),row(2)]
        if variant==9:parents=[dict(id=v('null'))];children=[dict(parent_col=v('null'),count=v('null'))]
        if variant==10:parents=[dict(id=v('i64','1'))];children=[dict(parent_id=u(1),count=u(9))]
        if variant==11:parents=[dict(id=t('1'))];children=[dict(parent_id=u(1),count=u(9))]
        if variant==12:children=[dict(parent_col=u(1),a=v('list',[u(1)]))]
        if variant==13:children=[dict(parent_col=u(1),count=v('i64','-3'))]
        add('aggregate shape '+str(variant),'aggregate',enhancements=[enhancement('aggregate',single=single)],parents=parents,responses=[children])
    options=[None,cache(),cache(False,True),cache(True,True,0,0),cache(True,True,ttl=900,child_ttl=2**64-1)]
    rich=dict(entity='IgnoredTarget',projection=['label'],expr_projection=[dict(alias='computed',expr=dict(kind='column',name='price'))],group=['kind'],order=[dict(field='price',direction=1)],slice=dict(offset=2,limit=1),relations=[dict(name='leafs')],partition='keep_partition',raw_projection=[dict(alias='raw',sql='1')],dynamic=[dict(alias='dyn',sql='2')],raw_criteria=['kind > 0'],search='search',comment='comment',raw_sql='raw statement',having=dict(kind='binary',op=2,left=dict(kind='column',name='count'),right=dict(kind='value',value=u(0))))
    for mode,parent_cache,child_cache in itertools.product(['aggregate','group','child'],options,options):
        q=copy.deepcopy(rich);q['cache']=child_cache
        add('query fields/cache',mode,cache=parent_cache,enhancements=[enhancement(mode,query=q)])
    for single,aggregates,group in itertools.product([False,True],[[],[dict(function=1,field='price',alias='total')],[dict(function=1,field='price',alias='total'),dict(function=4,field='price',alias='max')]], [[],['parent_id'],['kind','parent_id','parent_id']]):
        add('aggregate planning','aggregate',enhancements=[enhancement('aggregate',single=single,query=dict(entity='Ignored',aggregates=aggregates,group=group,filter=dict(kind='binary',op=0,left=dict(kind='column',name='kind'),right=dict(kind='value',value=t('a')))))])
    for mode,variant in itertools.product(['group','child'],range(11)):
        parents=[row(1,ref=u(1),object=t('old')),row(2,ref=u(2)),dict(object=t('keep missing'))];children=[row(1,label=t('first')),row(2,label=t('second'))]
        if variant==0:children=[]
        if variant==1:children=[row(1,label=t('first')),dict(id=v('i64','1'),label=t('last'))]
        if variant==2:children=[dict(label=t('no id'))]
        if variant==3:parents=[]
        if variant==4:parents=[{},dict(object=t('no join key'))]
        if variant==5:parents=[row(1,ref=u(1)),row(1,ref=u(1))]
        if variant==6:parents=[dict(id=v('null'),ref=v('null'))];children=[dict(id=v('null'),label=t('null matched'))]
        if variant==7:parents=[dict(id=t('1'),ref=t('1'))]
        if variant==8:children=[row(7,label=t('unmatched'))]
        if variant==9:children=[row(1,ref=u(9),object=v('object',dict(a=u(3))))]
        if variant==10:parents=[row(2,ref=u(2)),row(1,ref=u(1)),row(2,ref=u(2))]
        add('row merge '+str(variant),mode,parents=parents,responses=[children],enhancements=[enhancement(mode,query=dict(entity='Child',projection=['label']))])
    for mode,fail,pending,polls in itertools.product(['aggregate','group','child'],[0,1,2],[0,1],[0,1,2,6]):
        enhancements=[enhancement(mode),enhancement(mode)]
        if mode=='aggregate':enhancements[1]['alias']='next'
        if mode=='group':enhancements[1].update(property='next',storage='object')
        replies=[[dict(parent_id=u(1),count=u(3))],[dict(parent_id=u(1),count=u(5))]] if mode=='aggregate' else [[dict(id=v('i64','1'),label=t('merged first'))],[row(1,label=t('merged second'))]]
        add('sequential lifecycle',mode,enhancements=enhancements,responses=replies,fail_at=fail,pending=pending,polls=polls)
    for variant in range(6):
        es=entities();enh=enhancement('aggregate')
        if variant==0:es=[]
        if variant==1:es=es[:1]
        if variant==2:enh['relation']='absent'
        if variant==3:enh['query']['relations']=[dict(name='absent')]
        if variant==4:enh['query']['relations']=[dict(name='leafs')];es=es[:2]
        if variant==5:enh['query']['relations']=[dict(name='leafs',query=dict(entity='Leaf',relations=[dict(name='absent')]))]
        add('validation precedes empty rows','aggregate',entities=es,enhancements=[enh],parents=[])
    identities=[v('null'),dict(kind='typed_null',type='Text'),u(1),v('i64','1'),v('i64','-1'),t('1'),v('bool',False),v('f64','-0'),v('decimal','1.00'),v('date','2024-02-29'),v('timestamp','123'),v('json',{'a':1}),v('object',dict(a=u(1))),v('list',[u(1)])]
    for mode,key in itertools.product(['aggregate','group','child'],identities):
        response=dict(parent_id=key,count=u(4)) if mode=='aggregate' else dict(id=key,label=t('matched'))
        add('identity bucket',mode,parents=[dict(id=key,ref=key)],responses=[[response]])
    for mode in ['aggregate','group','child']:
        add('empty enhancement list',mode,enhancements=[])
    return data
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True);data=cases();observed=observe(args.rust,[c['input'] for c in data]);assert len(observed)==len(data)
    for c,o in zip(data,observed):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit'];Path(__file__).with_name('relation_enhancement_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,separators=(',',':')) for c in data)+'\n]}\n');print(f'Observed {len(data)} locked relation enhancement cases')
