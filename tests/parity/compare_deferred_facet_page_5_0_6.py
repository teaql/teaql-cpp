#!/usr/bin/env python3
"""Compare deferred composed execution against the original generated runtime."""
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

CORPUS = Path(__file__).with_name('deferred_facet_page_5_0_6_cases.json.gz')


def inputs():
    cases = [dict(mode=mode, optimization=optimization, gate=gate, action=action,
                  offset=offset, limit=limit, include_all=include_all)
             for mode, optimization, gate, action, offset, limit, include_all in itertools.product(
                 ['list', 'one', 'page'], ['plain', 'id_set', 'continuous'],
                 ['none', 'count', 'rows', 'facet'], ['resume', 'cancel', 'fail'],
                 [0, 1, 99], [0, 1, 2], [False, True])]
    assert len(cases) == 1944
    return cases


def identity():
    project = ROOT / 'tests/parity/rust_execution_shapes_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    wanted = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name not in {'teaql_execution_shapes_506', 'cpp-dynamic-json-core'}:
            assert resolved <= wanted.get(name, set()), name
    sdk = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((sdk / 'PROVENANCE.json').read_text())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert provenance['runtime_commit'] == baseline and not provenance['hand_edited']
    assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
    for name, digest in provenance['files'].items():
        assert hashlib.sha256((sdk / name).read_bytes()).hexdigest() == digest, name
    paths = ['tests/parity/rust_execution_shapes_5_0_6/Cargo.toml',
             'tests/parity/rust_execution_shapes_5_0_6/Cargo.lock',
             'tests/parity/rust_execution_shapes_5_0_6/src/bin/deferred_facet_page_5_0_6.rs',
             'tests/support/dynamic_json_schema.sql', 'tests/support/dynamic_json_dataset.sql']
    return dict(baseline=baseline, producer=provenance,
                sources={name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                         for name in paths},
                boundary='Original generated execution and provider/store algorithms; '
                         'test TeaqlRuntime SPI selects a gated real executor, including facets. '
                         'Native shared EntityQuery uses the same generated models and real provider. '
                         'No result, query, SQL, error or trace normalization; wall-clock timings excluded.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    current, cases = identity(), inputs()
    if args.record_source:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current, cases=[
            dict(input=i, expected=e) for i, e in zip(cases, expected)
        ]), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        assert all(corpus[key] == value for key, value in current.items())
        assert same(cases, [row['input'] for row in corpus['cases']])
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original observations changed')
    if not args.cpp:
        assert args.record_source
        print(f'Recorded {len(cases)} original deferred facet/page observations')
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(
            inputs=cases, original=expected, cpp=actual, failures=failures),
            ensure_ascii=False).encode(), mtime=0))
    for index in failures[:2]:
        print('Case:', cases[index])
        print('Differences:', [key for key in expected[index]
                               if key not in actual[index] or
                               not same(expected[index][key], actual[index][key])])
    print(f'Deferred facet/page execution: {len(cases) - len(failures)}/{len(cases)} original outcomes')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
