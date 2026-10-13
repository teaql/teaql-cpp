#!/usr/bin/env python3
"""Compare complete frozen public document snapshot validation and token calls."""
import argparse
from copy import deepcopy
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def base():
    return dict(snapshot=dict(model='model-v1', business='ORD-X1',
        scope=dict(document='doc-1', purpose='edit-order', type='Order', id=10, version=4),
        aggregate=dict(type='Order', id=10, version=4),
        entities=[dict(key='item:0', type='OrderItem', id=20, version=2,
                       loaded=['quantity', 'product_code'], writable=['quantity'], mutations=['update'])],
        relations=[dict(name='items', rows=['item:0'], completeness='complete', writable=True, mutations=['create', 'remove'])]),
        issued=1800000000, now=1800000000, lifetime=600, nanos=0, subject='alice',
        expected_type='Order', expected_purpose='edit-order', deny_issue=False, deny_consume=False, modification='none')


def payload_geometry(snapshot):
    """Locate fields in a known-valid stimulus, without implementing the decoder."""
    payload=bytearray(b'\x01'+bytes(32)+bytes(16))
    marks=[('version',0,1,'byte'),('actor',1,32,'bytes'),('issued',33,8,'i64'),('expires',41,8,'i64')]
    def integer(name,value,width=8,kind='i64'):
        marks.append((name,len(payload),width,kind));payload.extend(value.to_bytes(width,'big',signed=value<0))
    def text(name,value):
        value=value.encode();integer(name+'_length',len(value),4,'text_length');marks.append((name,len(payload),len(value),'text'));payload.extend(value)
    def identity(name,value):
        text(name+'_type',value['type']);integer(name+'_id',value['id']);integer(name+'_version',value['version'])
    def fields(name,values):
        values=sorted(set(values));integer(name+'_count',len(values),4,'count')
        for i,value in enumerate(values):text(f'{name}_{i}',value)
    def mutations(name,values):
        values=sorted(set({'update':1,'create':2,'remove':3}[v] for v in values));integer(name+'_count',len(values),4,'count')
        for i,value in enumerate(values):integer(f'{name}_{i}',value,1,'mutation')
    text('model',snapshot['model']);text('business',snapshot['business'])
    scope=snapshot['scope'];text('document',scope['document']);text('purpose',scope['purpose']);text('scope_type',scope['type'])
    integer('scope_id',scope['id']);integer('scope_version',scope['version']);identity('aggregate',snapshot['aggregate'])
    integer('entity_count',len(snapshot['entities']),4,'count')
    for i,value in enumerate(snapshot['entities']):
        text(f'entity_{i}_key',value['key']);identity(f'entity_{i}',value);fields(f'entity_{i}_loaded',value['loaded']);fields(f'entity_{i}_writable',value['writable']);mutations(f'entity_{i}_mutations',value['mutations'])
    integer('relation_count',len(snapshot['relations']),4,'count')
    for i,value in enumerate(snapshot['relations']):
        text(f'relation_{i}',value['name']);integer(f'completeness_{i}',1 if value['completeness']=='complete' else 2,1,'enum')
        integer(f'writable_{i}',int(value['writable']),1,'bool');mutations(f'relation_{i}_mutations',value['mutations']);integer(f'row_count_{i}',len(value['rows']),4,'count')
        for j,key in enumerate(value['rows']):text(f'row_{i}_{j}',key)
    return len(payload),marks


def cases():
    for subject, elapsed, modification in itertools.product(['alice', 'bob'], [-61, -60, 0, 599, 600, 601], ['none', 'tag', 'prefix', 'extra', 'missing_key']):
        v=base();v.update(subject=subject,now=v['issued']+elapsed,modification=modification);yield v
    for key, values in [('expected_type',['Order','Other',' ',' Order ']),('expected_purpose',['edit-order','other',' ',' edit-order ']),('lifetime',[0,1,9223372036854776]),('nanos',[0,1,999999999]),('deny_issue',[True]),('deny_consume',[True])]:
        for value in values:
            v=base();v[key]=value;yield v
    for path, values in [(['model'],['',' ',' model ']),(['business'],['',' ',' ORD-X1 ']),
                         (['entities',0,'key'],['',' ','item:0 ']),(['entities',0,'id'],[0,1]),
                         (['entities',0,'version'],[-1,0]),(['entities',0,'loaded'],[[],['quantity','quantity'],[' ','quantity']]),
                         (['entities',0,'writable'],[[],['missing']]),(['entities',0,'mutations'],[[],['update','update','create','remove']]),
                         (['relations',0,'rows'],[[],['missing'],['item:0','item:0']]),
                         (['relations',0,'completeness'],['partial']),(['relations',0,'writable'],[False])]:
        for value in values:
            v=base();target=v['snapshot']
            for key in path[:-1]:target=target[key]
            target[path[-1]]=value;yield v
    for count in [1023,1024,1025]:
        v=base();v['snapshot']['entities'][0]['loaded']=['quantity']+[f'f{i}' for i in range(count-1)];yield v
    for rows in [[],[base()['snapshot']['entities'][0]]*2]:
        v=base();v['snapshot']['entities']=deepcopy(rows);yield v
    for issue_key, decode_key, old_key in itertools.product(['k1','k2'], ['k1','k2'], ['retain','remove']):
        if old_key=='remove' and decode_key=='k1':continue
        v=base();v.update(issue_key=issue_key,decode_key=decode_key,old_key=old_key);yield v
    for issue_service, decode_service, issue_environment, decode_environment in itertools.product(
            ['order-service','another-service'],['order-service','another-service'],['test','production'],['test','production']):
        v=base();v.update(issue_service=issue_service,decode_service=decode_service,issue_environment=issue_environment,decode_environment=decode_environment);yield v
    for path in [['model'],['business'],['entities',0,'key'],['relations',0,'name']]:
        for length in [16383,16384,16385]:
            v=base();target=v['snapshot']
            for key in path[:-1]:target=target[key]
            target[path[-1]]='x'*length
            if path==['entities',0,'key']:v['snapshot']['relations'][0]['rows']=['x'*length]
            yield v
    for lifetime,nanos in [(0,1),(0,999999999),(1,1000000000),(1,4294967295)]:
        v=base();v.update(lifetime=lifetime,nanos=nanos);yield v
    for method, service, principal, failure in itertools.product(['open','accept'], [False,True], [False,True], ['none','revision','projection','validation','not_found']):
        yield dict(kind='service',method=method,service=service,principal=principal,failure=failure,
                   validate=True,type='Order',purpose='edit',business='ORD-1',lifetime=600,nanos=0,
                   token='token',command='command-1',body={'lines':[{'quantity':0,'text':'x'*128}],'active':False})
    for method, validate, key in itertools.product(['open','accept'],[False,True],['type','purpose','business','token','command','lifetime']):
        v=dict(kind='service',method=method,service=True,principal=True,failure='none',validate=validate,
               type='Order',purpose='edit',business='ORD-1',lifetime=600,nanos=0,token='token',command='command-1',body=None)
        v[key]=0 if key=='lifetime' else ' ';yield v
    size,marks=payload_geometry(base()['snapshot'])
    for offset in range(size):
        v=base();v.update(wire_operation='truncate',wire_offset=offset);yield v
    v=base();v.update(wire_operation='append',wire_offset=0);yield v
    for name,offset,width,kind in marks:
        if kind=='bytes':values=[0,255];operation='byte'
        elif kind=='text':values=[0,255];operation='byte'
        elif kind=='text_length':values=[0,16384,16385,4294967295];operation='u32'
        elif kind=='count':values=[0,1024,10000,10001,4294967295];operation='u32'
        elif kind in ['mutation','enum','bool','byte']:values=[0,1,2,3,4,255];operation='byte'
        elif name=='issued':values=[-1,1800000060,1800000061,9223372036854775807];operation='i64'
        elif name=='expires':values=[-1,1800000000,1800000001,9223372036854775807];operation='i64'
        else:values=[-1,0,1];operation='i64'
        for value in values:
            v=base();v.update(wire_operation=operation,wire_offset=offset,wire_value=value);yield v


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rust',required=True);p.add_argument('--cpp',required=True);args=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=list(cases());expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    failures=[(i,c,a,b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    for i,c,a,b in failures[:4]:print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if failures:raise SystemExit(f'{len(failures)}/{len(inputs)} document cases differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],note='Public frozen document snapshot constructors, issue/consume and actual UserContext open/accept calls. Exact tokens, independently re-authenticated malformed stimuli, authorization/callback order, payload/state/errors and request validation; no normalization. Native-only cancellation/lifetime adaptation remains separately qualified.',cases=[dict(input=c,expected=e) for c,e in zip(inputs,expected)])
    Path(__file__).with_name('document_round_trip_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} original/C++ document snapshot/token cases')


if __name__=='__main__':main()
