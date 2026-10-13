#!/usr/bin/env python3
"""Record direct locked Rust Context provider observations only after C++ agreement."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
ROOT = Path(__file__).resolve().parents[2]
def cases():
    base = dict(mode='database', namespace='profile', registered=True, duplicate=False,
                owner='Item', ids=[7,7,8], selection=None, stored={}, foreign=False, binding='bound')
    out = []
    for mode in ['database', 'fixture']:
        current = deepcopy(base)
        current['mode'] = mode
        if mode == 'fixture':
            current['stored'] = {'7': {'title': {'kind':'text','value':'private'}, 'count': {'kind':'null'}}}
        out.append(deepcopy(current))
        for name, variants in {
            'namespace':['',' ','\u2003','\u0085',' profile ','秘密','x'*4096],
            'registered':[False], 'duplicate':[True], 'owner':['Other',''],
            'ids':[[],[0],[7],[8,7,8],[2**64-1]],
            'selection':[[],[['title',5]],[['count',1]],[['title',1]],[['missing',5]],
                         [['#title',5]],[['_title',5]],[['title',5],['title',5]]],
            'foreign':[True], 'binding':['unbound','wrong_profile','wrong_schema'],
        }.items():
            for variant in variants:
                case=deepcopy(current);case[name]=variant;out.append(case)
        for payload in [{}, {'7':{}}, {'0':{'title':{'kind':'text','value':'zero'}}},
                        {'7':{'title':{'kind':'i64','value':'1'}}},
                        {'7':{'title':{'kind':'null'}}}, {'7':{'missing':{'kind':'null'}}},
                        {'7':{'count':{'kind':'i64','value':'0'}}}]:
            case=deepcopy(current);case['stored']=payload;out.append(case)
    return out

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True)
    args=parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=cases();expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    for i,(a,b) in enumerate(zip(expected,actual)):
        if not same(a,b):
            Path('/tmp/teaql_dynamic_provider_mismatch.json').write_text(json.dumps(dict(index=i,input=inputs[i],rust=a,cpp=b),indent=2))
            raise SystemExit(f'Provider mismatch {i}; see /tmp/teaql_dynamic_provider_mismatch.json')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    sources=['teaql-runtime/src/dynamic_fields.rs','teaql-data-service/src/dynamic_fields.rs']
    corpus=dict(baseline=lock['commit'],version=lock['version'],source=sources,
        source_sha256={s:hashlib.sha256((ROOT/'.reference/teaql-rs'/s).read_bytes()).hexdigest() for s in sources},
        note='Direct original database/fixture provider calls with real SQLite storage and a registered SQL executor. Private batch conversion/audit helper and graph-save orchestration are separate native/source-reviewed checks.',
        cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('dynamic_provider_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Compared and recorded {len(inputs)} Context provider observations')
if __name__=='__main__':main()
