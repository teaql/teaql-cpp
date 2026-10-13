#!/usr/bin/env python3
"""Compare unchanged 5.0.6 LoadedSnapshot/CompactRow dynamic read metadata."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT=Path(__file__).resolve().parents[2]
def text(v): return dict(kind='text',value=v)
def integer(v): return dict(kind='i64',value=str(v))
def dynamic(values,owner='Item',executor=None):
    out=dict(values=values,owner=owner)
    if executor is not None:out['executor']=executor
    return out
def cases():
    out=[]
    lookups=['id','version','db_0','field_63','field_64','field_129','lines','#stale','#extension','#count','_total','missing']
    for count in [2,63,64,65,130,141]:
        for full in [False,True]:
            for selected in [[],['id','lines','#stale'],['db_0','field_64','field_129','lines','#extension']]:
                for schema in [None,[],[['_total',1]]]:
                    payloads=[dynamic({}),dynamic({'extension':dict(kind='null')}),dynamic({'extension':text('changed')}),
                              dynamic({'count':integer(0)}),dynamic({'count':integer(0),'extension':text('雪')}),dynamic({}),dynamic({},'Other')]
                    out.append(dict(mode='snapshot',count=count,full=full,selected=selected,schema=schema,payloads=payloads,lookups=lookups))
    row=dict(native={'id':integer(7),'_total':integer(0)},schema=[['_total',1]],dynamic=dynamic({'extension':text('left')}),relation=False)
    for right_schema in [None,[],[['_total',1]],[['_total',5]],[['_extra',0]]]:
        for right_dynamic in [None,dynamic({}),dynamic({'extension':dict(kind='null')}),dynamic({'count':integer(1)}),dynamic({'extension':text('wrong')},executor=9)]:
            for right_relation in [False,True]:
                right=dict(native={'id':integer(99),'_total':integer(3)},schema=right_schema,dynamic=right_dynamic,relation=right_relation)
                out.append(dict(mode='rows',count=65,rows=[deepcopy(row),right],lookups=lookups,actions=['merge','take','clear']))
    for left_schema in [None,[],[['_total',1]]]:
        for operand in [dict(kind='null'),text('wrong'),dict(kind='bool',value=False),integer(0),dict(kind='json',value=1)]:
            left=deepcopy(row);left['schema']=left_schema;left['relation']=True
            right=dict(native={'id':integer(99),'_total':operand},schema=[['_total',1]],dynamic=None,relation=False)
            out.append(dict(mode='rows',count=2,rows=[left,right],lookups=lookups,actions=['merge','remove_relation','clear']))
    for left_dynamic in [None,dynamic({}),dynamic({'count':integer(0)})]:
        left=deepcopy(row);left['dynamic']=left_dynamic
        right=deepcopy(row);right['relation']=True
        out.append(dict(mode='rows',count=130,rows=[left,right],lookups=lookups,actions=['merge','merge','take','clear']))
    return out
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);args=parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    for i,(a,b) in enumerate(zip(expected,actual)):
        if not same(a,b):
            Path('/tmp/teaql_dynamic_carrier_mismatch.json').write_text(json.dumps(dict(index=i,input=inputs[i],rust=a,cpp=b),indent=2))
            raise SystemExit(f'Dynamic carrier mismatch {i}; see /tmp/teaql_dynamic_carrier_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());sources=['teaql-core/src/load_state.rs','teaql-core/src/query.rs','teaql-core/src/dynamic_fields.rs','teaql-core/src/dynamic_properties.rs']
    corpus=dict(baseline=lock['commit'],version=lock['version'],source=sources,
                source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest() for s in sources},
                note='Direct original LoadedSnapshot dynamic availability and CompactRow read-metadata/checked merge calls. No SQLite, runtime ledger or generated adapter qualification.',
                cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_carrier_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Compared and recorded {len(inputs)} dynamic carrier observations')
if __name__=='__main__':main()
