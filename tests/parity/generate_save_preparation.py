"""Observe original public extraction and private save preparation helpers."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe

def v(kind,value=None):
    out=dict(kind=kind)
    if value is not None:out['value']=value
    return out
def node(entity='Parent',values=None,operation=0,relations=None,dirty=None,old=None,comment=None):
    return dict(entity=entity,values={} if values is None else values,operation=operation,
                relations={} if relations is None else relations,dirty=dirty,old=old,comment=comment)
def rel(name,target,local='id',foreign='parent_id'):return dict(name=name,target=target,local=local,foreign=foreign)
def descriptors(required=False,action=None):
    out=[dict(name='Parent',required=required,relations=[rel('kids','Child'),rel('peer','Child','peer_id','id')]),
         dict(name='Child',required=required,relations=[rel('leaves','Leaf')]),
         dict(name='Leaf',required=required,relations=[])]
    if action is not None:
        for item in out:item['checker']=action
    return out
def case(op,**extra):
    out=dict(op=op,descriptors=descriptors(),seed=[],keys=[]);out.update(extra);return out
def key(entity,id):return dict(entity=entity,id=v('u64',str(id)))
def seed(entity,id,values=None,**flags):
    out=key(entity,id);out['values']={} if values is None else values;out.update(flags);return out
def inputs():
    cases=[]
    special=[{}, {'_comment':v('text','audit text')}, {'_comment':v('bool',True)},
             {'_dirty_fields':v('list',[v('text','z'),v('text','a'),v('text','a'),v('u64','2')])},
             {'_dirty_fields':v('text','ignored')},{'_original_values':v('object',{'version':v('i64','3')})},
             {'_original_values':v('null')}, {'_is_new':v('bool',True)}, {'_is_deleted':v('bool',True)},
             {'_is_new':v('bool',True),'_is_deleted':v('bool',True)},
             {'_is_new':v('text','true'),'_loaded_fields':v('list',[v('text','name')])}]
    children=[v('null'),v('typed_null'),v('object',{'id':v('u64','2'),'_comment':v('text','child')}),
              v('list',[]),v('list',[v('object',{'id':v('u64','2')})]),
              v('list',[v('object',{}),v('u64','3')]),v('bool',False),v('json',{'id':2})]
    for fields,child in itertools.product(special,children):
        values=dict(fields,id=v('u64','1'),kids=child,unknown=v('text','Preserved'))
        cases.append(case('extract',entity='Parent',values=values))
    for missing in ['Parent','Child','Leaf']:
        desc=[item for item in descriptors() if item['name']!=missing]
        values={'kids':v('object',{'leaves':v('object',{'id':v('u64','3')})})}
        cases.append(case('extract',entity='Parent',descriptors=desc,values=values))
    for fields,loaded,dirty,fresh,removed,comment in itertools.product(special,[None,[],['name','unknown']],
                                                 [None,[],['name']], [False,True],[False,True],[None,'','typed audit']):
        cases.append(case('typed',values=dict(fields,id=v('u64','1'),kids=v('object',{'id':v('u64','2')})),
                          loaded=loaded,dirty=dirty,fresh=fresh,removed=removed,comment=comment,old={'version':v('i64','5')}))
    for operation,action,required,root,idvalue in itertools.product(range(4),['none','fill','set_id','erase','error'],
                                                   [False,True],[False,True],[None,v('u64','1'),v('null')]):
        values={'name':v('text','old'),'_dirty_fields':v('list',[v('text','incoming')])}
        if idvalue is not None:values['id']=idvalue
        children=[node('Child',{'id':v('u64','2'),'name':v('text','child')},0,
                       {'leaves':[node('Leaf',{'id':v('u64','3')})]}),node('Child',{'id':v('u64','4')})]
        graph=node(values=values,operation=operation,relations={'kids':children},dirty=['name'])
        cases.append(case('preflight',node=graph,descriptors=descriptors(required,action),root=root,
                          keys=[key('Parent',1),key('Parent',77),key('Child',2),key('Leaf',3)]))
    for operation,dirty,identity,version in itertools.product(range(4),[None,[],['name','absent']],
                                           [None,v('null'),v('u64','2')],[None,v('i64','-2'),v('text','3'),v('typed_null')]):
        values={'name':v('text','changed')}
        if identity is not None:values['id']=identity
        old=None if version is None else {'version':version}
        graph=node(relations={'kids':[node('Child',values,operation,{'leaves':[node('Leaf',{'id':v('u64','3')},1)]},dirty,old)]})
        cases.append(case('merge',node=graph,keys=[key('Child',2),dict(entity='Child',id=v('null')),key('Leaf',3)]))
    for flags,existing,missing in itertools.product(['update','new','delete','newdelete','unversioned'],[False,True],['','Child','Leaf']):
        seeds=[seed('Child',2,{'parent_id':v('u64','1'),'name':v('text','changed')},**({} if flags=='unversioned' else {'version':3})),
               seed('Leaf',3,{'parent_id':v('u64','2')},new=True),seed('Child',4,{'parent_id':v('u64','9')})]
        seeds[0]['new']=flags in ['new','newdelete'];seeds[0]['delete']=flags in ['delete','newdelete']
        graph=node(values={'id':v('u64','1'),'peer_id':v('u64','2')},relations={'kids':[node('Child',{'id':v('u64','2')})]} if existing else {})
        desc=[item for item in descriptors() if item['name']!=missing]
        cases.append(case('hydrate',node=graph,seed=seeds,descriptors=desc,keys=[key('Child',2),key('Leaf',3)]))
    # Cycle discovery shares the visited set, and ambiguous references keep the
    # first path in sorted relation order.
    desc=descriptors();desc[1]['relations'].append(rel('parent','Parent','parent_id','id'))
    cases.append(case('hydrate',node=node(values={'id':v('u64','1')}),descriptors=desc,
                      seed=[seed('Child',2,{'parent_id':v('u64','1')},new=True),seed('Parent',1,{'name':v('text','cycle')},new=True)],keys=[key('Parent',1),key('Child',2)]))
    dup=node(values={'id':v('u64','1')},relations={'z':[node('Parent',{'id':v('u64','1')})],
                  'b':[node('Child',{'id':v('u64','2')})],'a':[node('Child',{'id':v('u64','2')})]})
    cases.append(case('merge',node=dup,keys=[key('Parent',1),key('Child',2)]))
    for entity,field,fresh,deleted in itertools.product(['Parent','Child'],['name','id','peer_id'],[False,True],[False,True]):
        cases.append(case('flags',node=node(values={'id':v('u64','1')}),key=key('Parent',1),
                          seed=[seed(entity,1,{field:v('text','value')},new=fresh,delete=deleted)],keys=[key(entity,1)]))
    return cases
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--rust',required=True);args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True);cases=inputs();outputs=observe(args.rust,cases)
    lock=json.loads((root/'upstream.lock.json').read_text());corpus=dict(baseline=lock['commit'],note='Original public typed extraction and private extraction/preflight/merge/hydration/location/cancellation/preservation methods via access-only archive bridges; fixture models/checkers do not reimplement runtime algorithms.',cases=[dict(input=i,expected=o)for i,o in zip(cases,outputs)])
    Path(__file__).with_name('save_preparation_cases.json').write_text(json.dumps(corpus,indent=2)+'\n');print(f'Recorded {len(cases)} save-preparation observations')
if __name__=='__main__':main()
