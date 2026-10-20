#!/usr/bin/env python3
"""Record/replay actual Business-ID sample rows, reports and timeless SQL diagnostics."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same
from verify_constant_closure_5_0_6 import registry_identities

OUTPUT = ROOT / 'tests/parity/business_sample_5_0_6_cases.json.gz'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    modules = ('plain', 'checker', 'behavior', 'combined')
    cases = [dict(module=mode, root=root, key=key, seed=seed, rounds=rounds, scale='Tiny')
             for mode, root, key, seed, rounds in itertools.product(
                 modules, ('normal', 'missing'), (True, False),
                 (0, 7, 18446744073709551596), (1, 2))]
    cases += [dict(module=mode, root='normal', key=key, seed=7, rounds=rounds, scale='Small')
              for mode, key, rounds in itertools.product(modules, (True, False), (1, 2))]
    cases += [dict(module=mode, root='normal', key=key, seed=7, rounds=2, scale='Tiny', fault=target)
              for mode, key, target in itertools.product(modules, (True, False), ('receipt', 'ticket'))]
    return cases


def provenance():
    support = json.loads((ROOT / 'tests/parity/business_sample_5_0_6_support.json').read_text())
    for path, expected in support['files'].items():
        assert digest(ROOT / path) == expected, f'Original observer support changed: {path}'
    directory = ROOT / 'tests/generator/parity/business_ids_rust'
    record = json.loads((directory / 'PROVENANCE.json').read_text())
    assert record['hand_edited'] is False
    assert record['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert record['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
    for path, expected in record['files'].items():
        assert digest(directory / path) == expected, path
    assert digest(ROOT / record['model']) == record['model_sha256']
    project = ROOT / 'tests/parity/rust_business_sample_5_0_6'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    return {str(path.relative_to(ROOT)): digest(path)
            for path in [directory / 'PROVENANCE.json', directory / 'lib/src/sample_data.rs',
                         project / 'Cargo.toml', project / 'Cargo.lock', project / 'src/main.rs']}


def observe(executable, cases):
    result = subprocess.run([str(executable.resolve())],
                            input=''.join(json.dumps(case) + '\n' for case in cases),
                            env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'),
                            text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'Observer failed ({result.returncode}): {result.stdout[-1000:]}\n{result.stderr[-4000:]}')
    outcomes = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(outcomes) == len(cases), 'Incomplete sample observations'
    return outcomes, result.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--cases', type=int, nargs='+', help='Select retained indices for focused imported/instrumented checks')
    args = parser.parse_args()
    source_inputs = provenance()
    cases = inputs()
    assert len(cases) == 128
    if args.record:
        if not args.rust or args.cpp or args.cases or OUTPUT.exists():
            parser.error('Record all cases through --rust into new immutable evidence only')
        expected, stderr = observe(args.rust, cases)
        OUTPUT.write_bytes(gzip.compress(json.dumps(dict(
            baseline=json.loads((ROOT / 'upstream.lock.json').read_text()),
            source_inputs=source_inputs,
            scope='Actual generated module/schema/sample entry. Complete rows/reports/SQL/trace/error outcomes; only wall-clock timing fields omitted as in the established sample contract.',
            cases=[dict(input=input_value, expected=value) for input_value, value in zip(cases, expected)],
            stderr=stderr), ensure_ascii=False).encode(), mtime=0))
        print(f'Recorded {len(cases)} complete original Business-ID sample scenarios')
        return
    record = json.loads(gzip.decompress(OUTPUT.read_bytes()))
    assert record['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())
    assert record['source_inputs'] == source_inputs
    assert same(cases, [case['input'] for case in record['cases']])
    indexes = args.cases or list(range(len(cases)))
    assert len(set(indexes)) == len(indexes) and all(0 <= index < len(cases) for index in indexes)
    selected = [cases[index] for index in indexes]
    expected = [record['cases'][index]['expected'] for index in indexes]
    if args.rust:
        assert same(observe(args.rust, selected)[0], expected), 'Original sample outcomes changed'
    if args.cpp:
        actual = observe(args.cpp, selected)[0]
        differences = [index for index, left, right in zip(indexes, actual, expected) if not same(left, right)]
        if differences:
            path = ROOT / 'docs/parity/business_sample_initial_comparison_5_0_6.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress(json.dumps(dict(indexes=indexes, actual=actual,
                    expected=expected, differences=differences), ensure_ascii=False).encode(), mtime=0))
            raise AssertionError(f'Business-ID sample differences: {differences}; raw outcomes retained')
    if not args.cpp and not args.rust:
        parser.error('Replay requires --cpp or --rust')
    rounds = sum(len(value.get('rounds', [])) for value in expected)
    logs = sum(len(round['logs']) for value in expected for round in value.get('rounds', []))
    print(f'Verified {len(indexes)} Business-ID sample scenarios, {rounds} invocations and {logs} exact ordered SQL diagnostics')


if __name__ == '__main__':
    main()
