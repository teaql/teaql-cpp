#!/usr/bin/env python3
"""Compare complete generated self/relation selection and removal through SQLite and typed load state."""
import argparse
import gzip
import hashlib
import itertools
import json
import sys
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS = Path(__file__).with_name('generated_selection_scope_5_0_6_cases.json.gz')

def inputs():
    operations=[[],['self'],['self_fields'],['without_parent'],['all'],['children'],['any'],
        ['all','all'],['children','self'],['relation','remove_relation'],
        ['relation_with','remove_relation'],['relation_with','remove_relation','all'],
        ['relation_with','self'],['raw','remove'],['raw','scalar','remove','self_fields'],
        ['group_alias','remove'],['computed','remove','self'],['raw','all'],
        ['children','remove_relation'],['remove_relation','children'],
        ['scalar','remove','remove'],['computed','raw_filter','all'],
        ['search','self'],['deleted','any']]
    return [dict(entity=entity,minimal=minimal,mode=mode,ops=ops)
            for entity,minimal,mode,ops in itertools.product(
                ['person','order','product'],[False,True],['rows','list'],operations)]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_selection_scope_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_selection_scope_506', 'cpp-dynamic-json-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    directory = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((directory / 'PROVENANCE.json').read_text())
    assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
    for name, digest in provenance['files'].items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest, name
    source_review = json.loads((ROOT / 'docs/parity/selection_scope_source_review_5_0_6.json').read_text())
    for name, digest in source_review['fixture_sha256'].items():
        assert hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest() == digest, name
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    __import__('os').environ['TEAQL_LOG_ENDPOINT']='off'
    cases = inputs()
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline, producer=provenance,
            cases=[dict(input=i, expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        assert corpus['baseline'] == baseline and corpus['producer'] == provenance
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases)), 'Original request observations changed'
    actual = observe(args.cpp, cases)
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT / 'docs/parity/generated_selection_scope_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Generated selection scope: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
