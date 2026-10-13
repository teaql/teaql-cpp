#!/usr/bin/env python3
"""Compare original 5.0.6 stream plans, batch validation and private write Debug."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def value(kind,data=None):return dict(kind=kind,**({'value':data}if data is not None else {}))
def cases():
    base=dict(mode='plan',namespace='trusted-profile',selection=None,store=dict(key=1,identity=7),
        validate=[dict(key=1,identity=7),dict(key=2,identity=7),dict(key=1,identity=8),dict(key=1,identity=None)],
        rows=[{'id':value('u64','7')},{'id':value('u64','7')},{'id':value('u64','8')}],ids=[7,7,8],stored={'7':{'name':value('text','payload')},'8':{}})
    out=[deepcopy(base)]
    for namespace in ['', ' ', '\u2003','\u0085',' profile ','秘密', 'n'*4096]:
        case=deepcopy(base);case['namespace']=namespace;out.append(case)
    for selected in [[],[['name',5]],[['count',1]],[['name',1]],[['count',5]],[['missing',5]],[['name',5],['name',5]],[['_readonly',5]],[['#name',5]]]:
        case=deepcopy(base);case['selection']=selected;out.append(case)
    for identity in [None,0,1,7,2**64-1]:
        case=deepcopy(base);case['store']['identity']=identity;out.append(case)
    for id_value in [None,value('null'),value('i64','-1'),value('u64','0'),value('bool',True),value('text','7'),value('f64','7.0'),value('json',{})]:
        case=deepcopy(base);case['rows'][0]={}if id_value is None else {'id':id_value};out.append(case)
    for ids in [[],[7],[7,8,8],[8,7,8],[7,7,9]]:
        case=deepcopy(base);case['ids']=ids;out.append(case)
    for stored in [{},{'7':{}},{'7':{},'8':{},'9':{}},{'7':{'name':value('null')},'8':{}},
                   {'7':{'name':value('i64','1')},'8':{}},{'7':{'count':value('i64','0')},'8':{}},
                   {'7':{'unknown':value('text','x')},'8':{}},{'7':{},'8':{'name':value('bool',False)}}]:
        case=deepcopy(base);case['stored']=stored;out.append(case)
    for changes in [{},{'name':None},{'name':value('null')},{'name':value('text','SECRET-PAYLOAD')},
                    {'name':value('text','SECRET-PAYLOAD'),'count':value('i64','999')}, {'unknown':value('text','秘密')}]:
        out.append(dict(mode='write_debug',namespace='SECRET-NAMESPACE',changes=changes))
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rust',required=True);p.add_argument('--cpp',required=True);args=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True);inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    for i,(a,b)in enumerate(zip(expected,actual)):
        if not same(a,b):Path('/tmp/teaql_dynamic_storage_mismatch.json').write_text(json.dumps(dict(index=i,input=inputs[i],rust=a,cpp=b),indent=2));raise SystemExit(f'Dynamic storage mismatch {i}; see /tmp/teaql_dynamic_storage_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());sources=['teaql-data-service/src/dynamic_fields.rs','teaql-core/src/dynamic_fields.rs','teaql-core/src/query.rs']
    data=dict(baseline=lock['commit'],version=lock['version'],source=sources,source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest()for s in sources},
        note='Direct original pure stream-plan, owner-id/batch merge and privacy-safe write Debug calls with a fake store. No real storage I/O, runtime provider or SQLite qualification.',cases=[dict(input=i,expected=o)for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_storage_5_0_6_cases.json').write_text(json.dumps(data,indent=2)+'\n');print(f'Compared and recorded {len(inputs)} dynamic storage plan observations')
if __name__=='__main__':main()
