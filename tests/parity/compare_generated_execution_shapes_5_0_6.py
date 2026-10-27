#!/usr/bin/env python3
"""Compare actual grouped/aggregate generated execution and complete SQL logs."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import sys
from compare_values import observe, same
from expression_cli_cases import require_cases

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

CORPUS = Path(__file__).with_name('generated_execution_shapes_5_0_6_cases.json.gz')
SHAPES = ['plain', 'group_id', 'group_field', 'count', 'sum', 'group_sum',
          'empty_group', 'count_collision', 'stat']


def inputs():
    result = []
    for entity, shape, minimal, mode, page, optimization, result_type in itertools.product(
            ['person', 'product', 'order', 'line'], SHAPES, [False, True],
            ['count', 'exists', 'first', 'one', 'page', 'list'],
            [(0, 0), (1, 1)], ['none', 'id_set', 'continuous'], ['native', 'person']):
        result.append(dict(entity=entity, shape=shape, minimal=minimal, mode=mode,
                           offset=page[0], limit=page[1], optimization=optimization,
                           return_type=result_type, max_ids=100, filters=['{}'],
                           missing_table=False))
    for entity, shape, minimal, mode, result_type in itertools.product(
            ['person', 'product', 'order', 'line'], SHAPES, [False, True],
            ['count', 'exists', 'first', 'one', 'page', 'list'], ['native', 'person']):
        result.append(dict(entity=entity, shape=shape, minimal=minimal, mode=mode,
                           offset=0, limit=1, optimization='none', return_type=result_type,
                           max_ids=100, filters=['{}'], missing_table=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_execution_shapes_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(lock) <= registry_identities(reference_lock)
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_execution_shapes_506', 'cpp-dynamic-json-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    producer_path = ROOT / 'tests/generator/parity/dynamic_json_rust'
    producer = json.loads((producer_path / 'PROVENANCE.json').read_text())
    for name, expected_hash in producer['files'].items():
        assert hashlib.sha256((producer_path / name).read_bytes()).hexdigest() == expected_hash
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    fixtures = {name: hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest()
                for name in ['dynamic_json_schema.sql', 'dynamic_json_dataset.sql']}
    cases = inputs()
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(
            dict(baseline=baseline, producer=producer, fixtures=fixtures,
                 cases=[dict(input=i, expected=e) for i, e in zip(cases, expected)]),
            ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        require_cases(corpus, len(cases), cases)
        assert corpus['baseline'] == baseline and corpus['producer'] == producer
        assert corpus['fixtures'] == fixtures
        assert same(cases, [x['input'] for x in corpus['cases']])
        expected = [x['expected'] for x in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original execution changed')
    actual = observe(args.cpp, cases)
    if len(actual) != len(cases):
        raise RuntimeError('Native observations must cover the complete input matrix')
    failures = [n for n, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record:
        archive = ROOT / 'docs/parity/generated_execution_shapes_initial_5_0_6.json.gz'
        assert not archive.exists()
        archive.write_bytes(gzip.compress(json.dumps(
            dict(inputs=cases, original=expected, cpp=actual, failures=failures),
            ensure_ascii=False).encode(), mtime=0))
    for n in failures[:3]:
        print('Case', n, cases[n], '\nRust:', expected[n], '\nC++:', actual[n])
    print(f'Grouped/aggregate generated execution: {len(cases) - len(failures)}/{len(cases)} '
          'exact original outcomes')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
