#!/usr/bin/env python3
"""Compare complete original generated inherited dynamic-JSON query semantics."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import sys
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS = Path(__file__).with_name('dynamic_json_inheritance_5_0_6_cases.json.gz')


def inputs():
    fields = {
        'platform': ['id', 'name', 'version', 'schools.name', 'schools.advisor.name'],
        'specialist': ['name', 'active', 'platform', 'platform_id', 'platform.name'],
        'school': ['name', 'rank', 'version', 'advisor', 'advisor_id', 'advisor.name',
                   'advisor.platform.name', 'enrollments.note'],
        'academy': ['name', 'rank', 'version', 'campus_code', 'platform', 'advisor_id',
                    'platform.name', 'advisor.name', 'enrollments.note'],
        'research_academy': ['name', 'rank', 'version', 'campus_code', 'budget', 'advisor',
                             'platform_id', 'platform.name', 'advisor.name',
                             'advisor.platform.name', 'enrollments.note'],
        'enrollment': ['note', 'school', 'school_id', 'school.name', 'school.advisor.name',
                       'school.advisor.platform.name'],
    }
    values = [None, '', '雪\u0000value', '__is_null__', '__is_not_null__', True, 7,
              [1, 9], [1, None, 3], [], {'__gt__': 2}]
    result = []
    for entity, names in fields.items():
        for field, operand, minimal, sliced in itertools.product(names, values, [False, True], [False, True]):
            result.append(dict(entity=entity, minimal=minimal, initial_slice=sliced,
                               filters=[json.dumps({field: operand}, ensure_ascii=False)]))
        for name, minimal, sliced in itertools.product(names, [False, True], [False, True]):
            for ordering in [name, {'field': name, 'useAsc': True},
                             [{'field': name, 'useAsc': False}, {'field': 'id', 'useAsc': True}]]:
                result.append(dict(entity=entity, minimal=minimal, initial_slice=sliced,
                                   filters=[json.dumps({'_orderBy': ordering, '_page': 2, '_pageSize': 3})]))
        for source, minimal, sliced in itertools.product(
                ['{', 'null', '[]', '"scalar"', '{"missing.field":1}',
                 '{"advisor.unknown":1}', '{"name":"A","name":"B"}',
                 '{"_start":1,"_size":2,"_pageSize":3,"_page":2}'],
                [False, True], [False, True]):
            result.append(dict(entity=entity, minimal=minimal, initial_slice=sliced,
                               filters=[source]))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_dynamic_json_inheritance_5_0_6/Cargo.lock'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    if not (registry_identities(lock) <= registry_identities(reference)):
        raise AssertionError()
    for name, resolved in versions(lock).items():
        if name not in {'teaql_dynamic_json_inheritance_506', 'cpp-inheritance-graph-core'}:
            if not (resolved <= versions(reference).get(name, set())):
                raise AssertionError(name)
    directory = ROOT / 'tests/generator/parity/inheritance_graph_rust'
    producer = json.loads((directory / 'PROVENANCE.json').read_text())
    for name, digest in producer['files'].items():
        if not (hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest):
            raise AssertionError()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cases = inputs()
    if args.record:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(
            dict(baseline=baseline, producer=producer,
                 cases=[dict(input=i, expected=e) for i, e in zip(cases, expected)]),
            ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if not (corpus['baseline'] == baseline and corpus['producer'] == producer):
            raise AssertionError()
        if not (same(cases, [c['input'] for c in corpus['cases']])):
            raise AssertionError()
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original queries changed')
    actual = observe(args.cpp, cases)
    failures = [n for n, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record:
        archive = ROOT / 'docs/parity/dynamic_json_inheritance_initial_5_0_6.json.gz'
        if not (not archive.exists()):
            raise AssertionError()
        archive.write_bytes(gzip.compress(json.dumps(
            dict(inputs=cases, original=expected, cpp=actual, failures=failures),
            ensure_ascii=False).encode(), mtime=0))
    for n in failures[:2]:
        print('Case', n, cases[n], '\nRust:', expected[n], '\nC++:', actual[n])
    print(f'Inherited dynamic JSON: {len(cases) - len(failures)}/{len(cases)} exact original constructions')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
