#!/usr/bin/env python3
"""Record unchanged 5.0.6 child and embedded relation fetching through public services."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def value(kind,data=None):return dict(kind=kind,**({'value':data} if data is not None else {}))
def cases():
    base=dict(mode='child',provider='fixture',selection=None,ids=[7,8],filter=None,chunk=2,missing_id=False,
        stored={'7':{'title':value('text','left'),'count':value('null')},'8':{'count':value('i64','0')}},
        children=[dict(selection=None,name='enhanced',filter=None)])
    out=[]
    for provider in ['fixture','database','missing']:
        current=deepcopy(base);current['provider']=provider;out.append(deepcopy(current))
        for ids in [[],[7],[8],[7,8,9]]:
            c=deepcopy(current);c['ids']=ids;out.append(c)
        for selection in [[],[['title',5]],[['count',1]],[['missing',5]],[['title',1]]]:
            c=deepcopy(current);c['children'][0]['selection']=selection;out.append(c)
        for filtered in [7,8,999]:
            c=deepcopy(current);c['children'][0]['filter']=filtered;out.append(c)
        for children in [[],[dict(selection=[['title',5]],name='first',filter=None),dict(selection=[['count',1]],name='second',filter=None)],
                         [dict(selection=None,name='first',filter=7),dict(selection=[['missing',5]],name='second',filter=8)]]:
            c=deepcopy(current);c['children']=children;out.append(c)
    relation=deepcopy(base);relation.update(mode='relation',depth=0,many=True,child_filter=None)
    for provider in ['fixture','database','missing']:
        for many in [True,False]:
            current=deepcopy(relation);current.update(provider=provider,many=many);out.append(deepcopy(current))
            for ids in [[],[7],[8],[7,8,9]]:
                c=deepcopy(current);c['ids']=ids;out.append(c)
            for filtered in [7,8,999]:
                c=deepcopy(current);c['child_filter']=filtered;out.append(c)
            for selection in [[],[['title',5]],[['count',1]],[['missing',5]],[['title',1]]]:
                c=deepcopy(current);c['selection']=selection;out.append(c)
            for depth in [1,2]:
                c=deepcopy(current);c['depth']=depth;out.append(c)
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rust',required=True);p.add_argument('--cpp',required=True);args=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    failures=[(i,a,b) for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if failures:
        Path('/tmp/teaql_dynamic_relation_mismatch.json').write_text(json.dumps([dict(index=i,input=inputs[i],rust=a,cpp=b) for i,a,b in failures],indent=2))
        raise SystemExit(f'{len(failures)} dynamic relation mismatches; see /tmp/teaql_dynamic_relation_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());sources=['teaql-runtime/src/data_service/relation.rs','teaql-runtime/src/data_service/resolved.rs','teaql-core/src/query.rs']
    corpus=dict(baseline=lock['commit'],version=lock['version'],source=sources,
        source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest() for s in sources},
        note='Direct original public raw-row child enhancements and embedded self-relations using a configured School descriptor and actual SQLite. Source metadata retention is observed, not normalized. Typed graph/fixture cleanup is separate native evidence; facets are outside this corpus.',cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_relation_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n');print(f'Compared and recorded {len(inputs)} dynamic relation observations')
if __name__=='__main__':main()
