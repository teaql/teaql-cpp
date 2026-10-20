#!/usr/bin/env python3
"""Compare typed scalar predicates through actual generated SQLite runtimes."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import sys

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

CORPUS = Path(__file__).with_name('query_predicate_typed_execution_5_0_6_cases.json.gz')
SCALARS = ['label', 'optional_label', 'notes', 'secret', 'amount', 'optional_amount',
           'height', 'ratio', 'counter', 'day', 'optional_day', 'moment', 'optional_moment']
OPERATIONS = ['eq', 'ne', 'gt', 'gte', 'lt', 'lte', 'before', 'after', 'between',
              'range', 'in', 'not_in', 'unknown', 'known', 'asc', 'desc', 'gbk_asc', 'gbk_desc']
NO_OPERAND = {'unknown', 'known', 'asc', 'desc', 'gbk_asc', 'gbk_desc', 'positive', 'negative'}


def inputs():
    result = []
    for field in ['id', 'version', *SCALARS, 'active', 'optional_active']:
        if field == 'id':
            operations = ['eq', 'ne', 'in', 'not_in', 'asc', 'desc', 'gbk_asc', 'gbk_desc']
        elif field == 'version':
            operations = ['asc', 'desc', 'gbk_asc', 'gbk_desc']
        elif field in {'active', 'optional_active'}:
            operations = ['positive', 'negative', 'unknown', 'known', 'asc', 'desc', 'gbk_asc', 'gbk_desc']
        else:
            operations = OPERATIONS
        if field in {'label', 'optional_label', 'notes', 'secret'}:
            native, upper = dict(kind='text', value='First'), dict(kind='text', value='Second')
        elif field in {'day', 'optional_day'}:
            native, upper = dict(kind='date', value='2026-10-14'), dict(kind='date', value='2026-10-15')
        elif field in {'moment', 'optional_moment'}:
            native, upper = dict(kind='timestamp', value='1791936000000'), dict(kind='timestamp', value='1792022400000')
        elif field in {'amount', 'optional_amount'}:
            native, upper = dict(kind='decimal', value='2.5'), dict(kind='decimal', value='4')
        elif field in {'height', 'ratio'}:
            native, upper = dict(kind='f64', value='2.5'), dict(kind='f64', value='4')
        else:
            native, upper = dict(kind='i64', value='3'), dict(kind='i64', value='12')
        values = [native, dict(kind='null'), dict(kind='text', value='雪\0bound'),
                  dict(kind='u64', value='18446744073709551615')]
        for operation in operations:
            operands = [dict(kind='null')] if operation in NO_OPERAND else values
            for operand, minimal, deleted, mode in itertools.product(
                    operands, [False, True], [False, True], ['rows', 'list']):
                result.append(dict(field=field, operation=operation, value=operand, upper=upper,
                                   minimal=minimal, deleted=deleted, mode=mode))
    return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_identity():
    checkpoint = json.loads((ROOT / 'docs/parity/query_predicate_text_checkpoint_5_0_6.json').read_text())
    for name in ('tests/parity/query_predicate_execution_probe.cpp',
                 'tests/parity/rust_query_predicate_execution_5_0_6/src/main.rs'):
        record = checkpoint['sources'][name]
        original = gzip.decompress((ROOT / record['snapshot']).read_bytes())
        assert hashlib.sha256(original).hexdigest() == record['sha256']
        expected = original.replace(b'query_field_branches_sql.sql', b'query_predicate_typed_sql.sql')
        if name.endswith('.cpp'):
            statement = b'    serviceRuntimeFromConnection(user_context, connection);'
            assert expected.count(statement) == 1
            expected = expected.replace(
                statement, b'    require_value(serviceRuntimeFromConnection(user_context, connection));', 1)
        assert (ROOT / name).read_bytes() == expected, (
            'Typed-schema observer changed beyond fixture path and exact startup Result check')
    project = ROOT / 'tests/parity/rust_query_predicate_execution_5_0_6'
    lock = project / 'Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_query_predicate_execution_506', 'cpp-query-branches-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    directory = ROOT / 'tests/generator/parity/query_field_branches_rust'
    provenance = json.loads((directory / 'PROVENANCE.json').read_text())
    assert digest(ROOT / provenance['model']) == provenance['model_sha256']
    for name, expected in provenance['files'].items():
        assert digest(directory / name) == expected, name
    cpp_directory = ROOT / 'tests/generated_query_field_branches'
    cpp_manifest = json.loads((cpp_directory / 'manifest.json').read_text())
    assert cpp_manifest['runtime_rust_baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert cpp_manifest['generator_reference_commit'] == provenance['generator_commit']
    assert cpp_manifest['model_sha256'] == provenance['model_sha256']
    for name, expected in cpp_manifest['files'].items():
        assert digest(cpp_directory / name) == expected, name
    paths = [project / 'Cargo.toml', lock, project / 'src/main.rs',
             ROOT / 'tests/parity/rust_values/src/fixture_value.rs',
             ROOT / 'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
             ROOT / 'tests/support/query_predicate_typed_sql.sql']
    return dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                producer=provenance, sources={p.relative_to(ROOT).as_posix(): digest(p) for p in paths})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    identity = source_identity()
    cases = inputs()
    if args.record:
        assert args.rust and not CORPUS.exists(), 'Preserve original observations'
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**identity,
            cases=[dict(input=i, expected=e) for i, e in zip(cases, expected)]),
            ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for name, item in identity.items():
            assert corpus[name] == item, name
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases)), 'Original outcomes changed'
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record:
        report = ROOT / 'docs/parity/query_predicate_typed_execution_initial_5_0_6.json.gz'
        assert not report.exists()
        report.write_bytes(gzip.compress(json.dumps(dict(inputs=cases, original=expected,
            cpp=actual, failures=failures), ensure_ascii=False).encode(), mtime=0))
    for index in failures[:3]:
        print('Case', index, cases[index])
        print('Rust:', expected[index])
        print('C++:', actual[index])
    print(f'Query predicate SQLite execution: {len(cases)-len(failures)}/{len(cases)} exact outcomes')
    print(f'Original: {sum(e["error"] is None for e in expected)} successes, '
          f'{sum(e["error"] is not None for e in expected)} errors, '
          f'{sum(len(e["logs"]) for e in expected)} ordered SQL logs')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
