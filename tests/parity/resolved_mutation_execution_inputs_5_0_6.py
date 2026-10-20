"""Reach real resolved mutation/event execution using trusted bound intent."""
from copy import deepcopy
from itertools import product
import gzip
import json
from pathlib import Path


def inputs():
    retained=json.loads(gzip.decompress(Path(__file__).with_name('runtime_mutation_5_0_6_cases.json.gz').read_bytes()))
    rows=[deepcopy(case['input']) for case in retained['cases']]
    template=deepcopy(rows[0])
    template.update(bound_intent=True,layer='entity',cache=False,log=False,counts=[1,1],
                    fail=-1,event_fail=-1,version=3,old=None,hook='none',policy='none')
    old_complete=dict(name=dict(kind='text',value='old'),other=dict(kind='text',value='retained'))
    old_partial=dict(other=dict(kind='text',value='retained'))
    shapes=[('entity',entry) for entry in ['insert','update','delete','recover']]
    shapes += [('prepared',entry) for entry in ['insert','update']]
    for (layer,entry),(counts,failure),event_failure,old,trace_mode,version in product(
        shapes,[([0],-1),([1],-1),([1],0)],[-1,0],
        [None,{},old_complete,old_partial],['none','command','both'],[None,3]):
        row=deepcopy(template)
        row.update(layer=layer,entry=entry,counts=counts,fail=failure,event_fail=event_failure,
                   old=deepcopy(old),version=version)
        if trace_mode=='none':row['trace']=[]
        if trace_mode!='both':row['lineage']=[]
        rows.append(row)
    for entry,hook,policy,fix,metadata,entity in product(
        ['insert','update','delete','recover'],['none','set','error'],['none','set','error'],
        [False,True],[False,True],['Order','Physical']):
        row=deepcopy(template)
        row.update(entry=entry,hook=hook,policy=policy,fix=fix,metadata=metadata,entity=entity)
        rows.append(row)
    for entry,length,id_count,version_mode,trace_mode,(counts,failure,event_failure) in product(
        ['batch_insert','batch_update'],range(3),[0,1,2,3],['empty','partial','full'],
        ['empty','partial'],[([1,1],-1,-1),([0,0],-1,-1),([1,1],0,-1),
                            ([1,1],1,-1),([1,1],-1,0),([1,1],-1,1)]):
        row=deepcopy(template)
        row.update(layer='prepared',entry=entry,counts=counts,fail=failure,event_fail=event_failure)
        row['batch']=row['batch'][:length]
        row['ids']=(row['ids']+[dict(kind='u64',value='9')])[:id_count]
        row['versions']=[] if version_mode=='empty' else [None] if version_mode=='partial' else [3,4]
        row['traces']=[] if trace_mode=='empty' else row['traces'][:1]
        rows.append(row)
    return rows
