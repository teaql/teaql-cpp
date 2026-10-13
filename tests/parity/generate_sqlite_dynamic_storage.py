#!/usr/bin/env python3
"""Compare real frozen Rust/C++ SQLite dynamic stores and transaction reads."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def v(kind,data=None,**kw):return dict(kind=kind,**({'value':data}if data is not None else {}),**kw)
def case(dtype,operand,rollback=False):
    return dict(fields=[['field',dtype]],revision='v1',namespace='profile',ids=['7','8'],selection=None,
        writes=[dict(namespace='profile',id='7',changes={'field':operand})],rollback=rollback,corrupt=None)
def cases():
    out=[]
    operands=[v('bool',False),v('i64',str(-(2**63))),v('u64',str(2**64-1)),v('f64','-0.0'),v('decimal','12345.6700'),v('text','雪\0data'),v('text','x'*8192),v('json',{'nested':[None,False,7]}),v('date','2026-10-11'),v('timestamp',str(-(2**63)))]
    for dtype,operand in enumerate(operands):
        for rollback in [False,True]:
            for value in [operand,v('null'),v('typed_null',type=dtype),None]:out.append(case(dtype,value,rollback))
    for number in ['0.0','-0.0','1.0','0.125','1e-308','1.7976931348623157e308','NaN','inf','-inf']:
        out.append(case(3,v('f64',number)))
    for namespace in ['', ' ', '\u2003',' profile ','秘密']:
        c=case(5,v('text','x'));c['writes'][0]['namespace']=namespace;c['namespace']=namespace;out.append(c)
    for owner in ['0','1',str(2**64-1)]:
        c=case(5,v('text','x'));c['writes'][0]['id']=owner;c['ids']=[owner,'7'];out.append(c)
    for selection in [[],[['field',5]],[['field',1]],[['missing',5]]]:
        c=case(5,v('text','x'));c['selection']=selection;out.append(c)
    c=case(5,v('text','first'));c['writes'].append(dict(namespace='profile',id='7',changes={'field':None}));out.append(c)
    c=case(5,v('text','first'));c['writes'].append(dict(namespace='profile',id='8',changes={'field':v('bool',True)}));out.append(c)
    for count in [0,1,400,401,805]:
        c=case(5,v('text','batch'));c['ids']=[str(i)for i in range(1,count+1)];out.append(c)
    updates=["definition_revision='stale'","data_type='I64'","payload='invalid SECRET'","payload='null'","payload='7'","payload='\"new text\"'","code='unknown'"]
    for change in updates:
        c=case(5,v('text','original'));c['corrupt']='UPDATE teaql_dynamic_field_storage_v1 SET '+change;out.append(c)
    for payload in ['"8000000000000000"','"+8000000000000000"','"0"','"7ff0000000000000"','"ffffffffffffffff"','"invalid"','0']:
        c=case(3,v('f64','1.0'));c['corrupt']="UPDATE teaql_dynamic_field_storage_v1 SET payload='"+payload+"'";out.append(c)
    c=case(5,v('text','x'));c['corrupt']='DROP TABLE teaql_dynamic_field_storage_v1';out.append(c)
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rust',required=True);p.add_argument('--cpp',required=True);args=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True);inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    for i,(a,b)in enumerate(zip(expected,actual)):
        if not same(a,b):Path('/tmp/teaql_sqlite_dynamic_mismatch.json').write_text(json.dumps(dict(index=i,input=inputs[i],rust=a,cpp=b),indent=2));raise SystemExit(f'SQLite dynamic mismatch {i}; see /tmp/teaql_sqlite_dynamic_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());sources=['teaql-provider-sqlite/src/dynamic_fields.rs','teaql-provider-sqlite/src/transaction.rs','teaql-core/src/dynamic_fields.rs']
    data=dict(baseline=lock['commit'],version=lock['version'],source=sources,source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest()for s in sources},
        note='Real original/C++ SQLite stores: validation, apply, typed transaction/ambient reads, commit/rollback, 400-id batches and malformed storage. No generated graph save, audit or cursor enhancement qualification.',cases=[dict(input=i,expected=o)for i,o in zip(inputs,expected)])
    Path(__file__).with_name('sqlite_dynamic_storage_5_0_6_cases.json').write_text(json.dumps(data,indent=2)+'\n');print(f'Compared and recorded {len(inputs)} real SQLite dynamic storage observations')
if __name__=='__main__':main()
