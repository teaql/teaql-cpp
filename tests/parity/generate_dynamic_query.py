#!/usr/bin/env python3
"""Compare frozen typed root hydration and real SQLite cursor enhancement."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]
def value(kind,data=None):return {'kind':kind,**({'value':data} if data is not None else {})}
def cases():
    base=dict(mode='root',provider='fixture',selection=None,ids=[7,8],filter=None,chunk=2,
        missing_id=False,stored={'7':{'title':value('text','extensions'),'count':value('null')}})
    out=[]
    for mode in ['root','enhanced','stream']:
        for provider in ['fixture','database','missing']:
            current=deepcopy(base);current.update(mode=mode,provider=provider);out.append(deepcopy(current))
            for selected in [[],[['title',5]],[['count',1]],[['title',5],['count',1]],[['title',1]],[['missing',5]],[['_property',5]],[['#title',5]],[['title',5],['title',5]]]:
                case=deepcopy(current);case['selection']=selected;out.append(case)
            for ids in [[],[7],[8],[7,8,9,10,11]]:
                case=deepcopy(current);case['ids']=ids;out.append(case)
            for filtered in [7,8,999]:
                case=deepcopy(current);case['filter']=filtered;out.append(case)
            for payload in [{},{'7':{}},{'7':{'title':value('null')}},{'7':{'count':value('i64','0')}},
                {'7':{'title':value('text','秘密')}},{'7':{'title':value('text','x'*4096)}},
                {'7':{'title':value('text','left')},'8':{'count':value('i64','-7')}},
                {'7':{'title':value('text','left')},'8':{'title':value('text','right')}}]:
                case=deepcopy(current);case['stored']=payload;out.append(case)
            if mode=='stream':
                case=deepcopy(current);case['missing_id']=True;out.append(case)
                for chunk in [0,1,3,1000]:
                    case=deepcopy(current);case['chunk']=chunk;out.append(case)
    for payload in [{'title':value('bool',False)},{'count':value('text','1')},{'unknown':value('null')}]:
        case=deepcopy(base);case['stored']={'7':payload};out.append(case)
    for payloads in [[],[{}],[{'title':value('text','x')}],
        [{'title':value('text','left')},{'title':value('text','right')}],
        [{'title':value('null')},{'title':value('text','right')}],
        [{'title':value('text','left')},{'count':value('i64','0')}], [{},{}]]:
        case=deepcopy(base);case.update(mode='decode',rows=[dict(id=7,values=v) for v in payloads]);out.append(case)
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rust',required=True);p.add_argument('--cpp',required=True);args=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    failures=[(i,a,b) for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if failures:
        Path('/tmp/teaql_dynamic_query_mismatch.json').write_text(json.dumps([dict(index=i,input=inputs[i],rust=a,cpp=b) for i,a,b in failures],indent=2))
        raise SystemExit(f'{len(failures)} hydration mismatches; see /tmp/teaql_dynamic_query_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    sources=['teaql-core/src/query.rs','teaql-runtime/src/registry.rs','teaql-runtime/src/data_service/resolved.rs','teaql-provider-sqlite/src/lib.rs']
    data=dict(baseline=lock['commit'],version=lock['version'],source=sources,
        source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest() for s in sources},
        note='Actual frozen School typed root/enhanced queries, central decoder and SQLite cursor enhancement. C++ handwritten carrier retains the School indexed geometry; native query fixture uses only name/id/version. No full generated School, relation loading, graph save or logging qualification.',
        cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_query_5_0_6_cases.json').write_text(json.dumps(data,indent=2)+'\n');print(f'Compared and recorded {len(inputs)} dynamic query observations')
if __name__=='__main__':main()
