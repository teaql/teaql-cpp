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
from expression_cli_cases import require_cases

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS = Path(__file__).with_name('relation_selection_inputs_5_0_6_cases.json.gz')

def inputs():
    operations=[['relation_with'],['relation_with','self'],
        ['relation_with','remove_relation'],['relation_with','remove_relation','all'],
        ['relation_with','relation_with'],['computed','relation_with','raw_filter'],
        ['deleted','relation_with'],['raw','relation_with']]
    return [dict(entity=entity,minimal=minimal,mode=mode,ops=ops,
                 conversion=conversion,child_mode=child_mode)
            for entity,minimal,mode,ops,conversion,child_mode in itertools.product(
                ['person','order','product'],[False,True],['rows','list'],operations,
                ['request','selection','query'],['plain','options','deleted','slice'])]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_relation_selection_inputs_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_relation_selection_inputs_506', 'cpp-dynamic-json-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    directory = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((directory / 'PROVENANCE.json').read_text())
    assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
    for name, digest in provenance['files'].items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest, name
    source_review = json.loads((ROOT / 'docs/parity/relation_selection_inputs_source_review_5_0_6.json').read_text())
    for method in source_review['methods']:
        assert method['body'] in (ROOT / method['source']).read_text(), method['name']
        assert hashlib.sha256(method['body'].encode()).hexdigest() == method['sha256']
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
        require_cases(corpus, len(cases), cases)
        assert corpus['baseline'] == baseline and corpus['producer'] == provenance
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases)), 'Original request observations changed'
    actual = observe(args.cpp, cases)
    if len(actual) != len(cases):
        raise RuntimeError('Native observations must cover the complete input matrix')
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT / 'docs/parity/relation_selection_inputs_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Runtime relation selection inputs: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
