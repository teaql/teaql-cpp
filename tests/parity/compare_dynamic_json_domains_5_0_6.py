#!/usr/bin/env python3
"""Compare actual scalar/optional/temporal dynamic-JSON requests and SQLite values."""
import argparse
import gzip
import hashlib
import itertools
import os
import json
import sys
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS = Path(__file__).with_name('dynamic_json_domains_5_0_6_cases.json.gz')

def inputs():
    fields={'order_status':['id','version','name','code','platform','platform_id','platform.name'],
            'record':['name','phase','tag','status','status_id','status.name','status.code','platform.name'],
            'person':['name','phase','status','status_id','status.code'],
            'receipt':['name','receipt_number','tenant','tenant_id','tenant.name'],
            'ticket':['name','ticket_number','tenant','tenant_id','tenant.name']}
    values=[None,'','Draft','DRAFT','done','red','__IS_NULL__','__IS_NOT_NULL__',11,22,[11,22],['DRAFT','DONE'],[],{'__gt__':11},'RCP','TKT']
    result=[]
    for entity,names in fields.items():
        for field,operand,minimal,mode,order in itertools.product(names,values,[False,True],['rows','count','exists'],['none','asc_page']):
            criteria={field:operand}
            if order=='asc_page':criteria.update(_orderBy={'field':field,'useAsc':True},_page=2,_pageSize=1)
            result.append(dict(entity=entity,minimal=minimal,mode=mode,filters=[json.dumps(criteria)]))
        for source,minimal,mode in itertools.product(['{','null','[]','{"unknown":1}','{"_start":1,"_size":2,"_pageSize":1,"_page":2}'],[False,True],['rows','count','exists']):
            result.append(dict(entity=entity,minimal=minimal,mode=mode,filters=[source]))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_dynamic_json_domains_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_dynamic_json_domains_506', 'cpp-candidates-core', 'cpp-business-ids-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    provenance={}
    for fixture in ['candidates','business_ids']:
        directory=ROOT/'tests/generator/parity'/(fixture+'_rust')
        item=json.loads((directory/'PROVENANCE.json').read_text())
        assert hashlib.sha256((ROOT/item['model']).read_bytes()).hexdigest()==item['model_sha256']
        for name,digest in item['files'].items():
            assert hashlib.sha256((directory/name).read_bytes()).hexdigest()==digest,name
        provenance[fixture]=item
    # Public deterministic fixture key for original generated startup; no ID allocation is invoked.
    os.environ['TEAQL_BUSINESS_ID_KEY_V1_HEX']='00'*32
    os.environ['TEAQL_BUSINESS_ID_KEY_VERSION']='1'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    schema_hash = hashlib.sha256((ROOT / 'tests/support/dynamic_json_domain_models.sql').read_bytes()).hexdigest()
    cases = inputs()
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline, producer=provenance, schema_sha256=schema_hash,
            cases=[dict(input=i, expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        assert corpus['baseline'] == baseline and corpus['producer'] == provenance
        assert corpus['schema_sha256'] == schema_hash
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original request observations changed')
    actual = observe(args.cpp, cases)
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT / 'docs/parity/dynamic_json_domains_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Dynamic JSON domain SQL: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
