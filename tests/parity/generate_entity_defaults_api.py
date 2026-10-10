#!/usr/bin/env python3
"""Observe original Entity/BaseEntity defaults and dynamic mutation sequences."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    def v(kind,data=None): return dict(kind=kind,value=data)
    pool=[v('null'),v('typed_null'),v('bool',True),v('bool',False),
          *[v('i64',str(n)) for n in (-9223372036854775808,-1,0,9223372036854775807)],
          *[v('u64',str(n)) for n in (0,18446744073709551615)],
          *[v('f64',n) for n in ('NaN','inf','-inf','-0.0','129.95')],
          v('decimal','123.450'),v('text','42'),v('text','雪\n\0'),
          v('date','2024-02-29'),v('timestamp','-1'),v('json',{'x':[None,True]}),
          v('list',[v('i64','-1'),v('text','nested')]),v('object',{'x':v('bool',True)})]
    cases=[]
    for item in pool:
        for key in ('payload','id','version',''):
            ops=[dict(kind='get',key=key),dict(kind='with',key=key,value=item),
                 dict(kind='get',key=key),dict(kind='put',key=key,value=v('text','replacement')),
                 dict(kind='remove',key=key),dict(kind='remove',key=key),
                 dict(kind='put',key=key,value=item),dict(kind='id',value='18446744073709551615'),
                 dict(kind='version',value='-9223372036854775808')]
            cases.append(dict(id='7',version='-3',ops=ops))
    cases.append(dict(id='0',version='0',ops=[]))
    for entity in ['', 'Edge', '雪\0']:
        for message in ['', 'private\n\0message', 'm'*8192]:
            cases.append(dict(kind='error_source',entity=entity,message=message))
    return cases


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--rust',required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    cases=inputs();outputs=observe(args.rust,cases)
    corpus=dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
                note='Original Entity/BaseEntity defaults and dynamic methods; full ordered states.',
                cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    Path(__file__).with_name('entity_defaults_api_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Recorded {len(cases)} original Entity default/dynamic sequences')


if __name__=='__main__': main()
