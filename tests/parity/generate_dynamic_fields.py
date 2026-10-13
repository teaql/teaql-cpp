#!/usr/bin/env python3
"""Observe unchanged Rust 5.0.6 dynamic-field APIs and compare C++ contracts."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]

def cases():
    base = dict(owner='Item', revision='v1', definitions=[['name',5],['count',1],['payload',7]],
                mode='rows', rows=[{'name':dict(kind='text',value='first')},{'name':dict(kind='null')},{}],
                lookups=['name','count','payload','missing','_readonly','#extension'], actions=[])
    result = [deepcopy(base)]
    for key,val in [('owner',''),('owner','\u2003'),('revision',''),('revision','\u0085'),
                    ('owner','雪'),('revision','v'*4096)]:
        case=deepcopy(base);case[key]=val;result.append(case)
    for name in ['', ' ', '\u3000', '_readonly', '#field', '雪', ' name ', 'x'*4096]:
        case=deepcopy(base);case['definitions']=[[name,5]];case['rows']=[{name:dict(kind='text',value='v')}];case['lookups']=[name];result.append(case)
    case=deepcopy(base);case['definitions'].append(['name',5]);result.append(case)
    for selected in [None,[],[['name',5]],[['count',5]],[['missing',1]],[['name',5],['name',5]],[['_readonly',5]],[['#field',5]],[['\u2003',5]]]:
        case=deepcopy(base);case.update(mode='selection',selection=selected);result.append(case)
    case=deepcopy(base);case['mode']='binding';result.append(case)
    operands=[dict(kind='null'),dict(kind='typed_null'),dict(kind='bool',value=False),
              dict(kind='i64',value='0'),dict(kind='u64',value='0'),dict(kind='f64',value='-0.0'),
              dict(kind='decimal',value='1.25'),dict(kind='text',value='雪'),dict(kind='json',value={'x':[1,None]}),
              dict(kind='date',value='2026-10-11'),dict(kind='timestamp',value='1791676800000'),
              dict(kind='list',value=[]),dict(kind='object',value={})]
    for declared in range(10):
        for operand in operands:
            case=deepcopy(base);case['definitions']=[['field',declared]];case['lookups']=['field','unknown'];case['rows']=[{'field':operand}];result.append(case)
            case=deepcopy(base);case['definitions']=[['field',declared]];case['lookups']=['field'];case['rows']=[{},{}];case['actions']=[dict(row=0,op='assign',code='field',value=operand)];result.append(case)
    for code in ['name','count','missing']:
        case=deepcopy(base);case['actions']=[dict(row=0,op='delete',code=code)];result.append(case)
    action_sets=[
        [dict(row=0,op='assign',code='name',value=dict(kind='text',value='next'))],
        [dict(row=0,op='merge',other=1)],
        [dict(row=0,op='merge',other=2)],
        [dict(row=2,op='merge',other=0)],
        [dict(row=0,op='assign',code='count',value=dict(kind='i64',value='0')),dict(row=0,op='delete',code='count')],
        [dict(row=2,op='merge',other=0),dict(row=1,op='assign',code='count',value=dict(kind='i64',value='7')),dict(row=0,op='merge',other=1),dict(row=2,op='merge',other=1)],
    ]
    for actions in action_sets:
        case=deepcopy(base);case['actions']=actions;result.append(case)
    case=deepcopy(base)
    case['rows']=[{'name':dict(kind='text',value='a')},{'count':dict(kind='i64',value='7')},
                  {'name':dict(kind='null')},{'name':dict(kind='text',value='b')}]
    case['actions']=[dict(row=i,op='merge',other=1) for i in [0,2,3]]
    result.append(case)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True)
    parser.add_argument('--cpp',required=True)
    args=parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    for index,(a,b) in enumerate(zip(expected,actual)):
        if not same(a,b):
            raise SystemExit(f'Dynamic field mismatch {index}:\n{inputs[index]}\nRust {a}\nC++ {b}')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    source='teaql-core/src/dynamic_fields.rs'
    corpus=dict(baseline=lock['commit'],version=lock['version'],source=source,
                source_sha256=hashlib.sha256((ROOT/'.reference/teaql-rs'/source).read_bytes()).hexdigest(),
                note='Direct original definitions, selection, value validation, batch shape identity and mutations/merge calls. No SQLite persistence or generated API qualification.',
                cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_fields_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2,ensure_ascii=True)+'\n')
    print(f'Compared and recorded {len(inputs)} Rust 5.0.6 dynamic-field observations')

if __name__=='__main__':
    main()
