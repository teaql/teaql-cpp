#!/usr/bin/env python3
"""Replay actual configured-type sample entries with unchanged typed operands."""
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
from verify_legacy_observation_identity_5_0_6 import verify_record

BASE = ROOT / 'tests/generator/parity/sample_type_configuration_contracts'
CASES = json.loads((BASE / 'cases.json').read_text())
CORPUS = ROOT / 'tests/parity/sample_types_5_0_6_cases.json.gz'
FOCUSED = ROOT / 'tests/parity/sample_types_5_0_6_focused_cases.json.gz'
SELECTION = ROOT / 'tests/parity/sample_types_5_0_6_instrumented_selection.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    modules = ('plain', 'checker', 'behavior', 'combined')
    cases = [dict(case=case, module=module, root=root, seed=seed, rounds=rounds, scale='Tiny', fault=False)
             for case, module, root, seed, rounds in itertools.product(CASES, modules,
                 ('normal', 'missing'), (0, 7, 18446744073709551596), (1, 2))]
    cases += [dict(case=case, module=module, root='normal', seed=7, rounds=rounds, scale='Small', fault=False)
              for case, module, rounds in itertools.product(CASES, modules, (1, 2))]
    cases += [dict(case=case, module=module, root='normal', seed=7, rounds=2, scale='Tiny', fault=True)
              for case, module in itertools.product(CASES, modules)]
    return cases


def provenance():
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())
    result = {}
    for case in CASES:
        directory = BASE / case / 'rust'
        record = json.loads((directory / 'PROVENANCE.json').read_text())
        assert not record['hand_edited'] and record['runtime_commit'] == baseline['commit']
        assert record['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
        assert digest(ROOT / record['model']) == record['model_sha256']
        actual = {str(p.relative_to(directory)): digest(p) for p in directory.rglob('*')
                  if p.is_file() and p.name != 'PROVENANCE.json'}
        assert actual == record['files']
        project = ROOT / 'tests/parity/rust_sample_type_5_0_6' / case
        assert registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
        for path in [directory / 'PROVENANCE.json', project / 'Cargo.toml', project / 'Cargo.lock']:
            result[str(path.relative_to(ROOT))] = digest(path)
    for path in [BASE / 'cases.json', ROOT / 'tests/parity/rust_sample_type_5_0_6/main.rs',
                 ROOT / 'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs']:
        result[str(path.relative_to(ROOT))] = digest(path)
    return result


def observe(cases, cpp=None, rust_dir=None):
    outputs = [None] * len(cases)
    stderr = {}
    for case in CASES:
        selected = [(i, value) for i, value in enumerate(cases) if value['case'] == case]
        if not selected:
            continue
        executable = cpp if cpp else rust_dir / ('sample_type_' + case)
        result = subprocess.run([str(executable.resolve())],
                                input=''.join(json.dumps(value) + '\n' for _, value in selected),
                                text=True, capture_output=True, env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
        if result.returncode:
            raise RuntimeError(f'{case}: observer exited {result.returncode}: {result.stderr[-4000:]}')
        actual = [json.loads(line) for line in result.stdout.splitlines()]
        assert len(actual) == len(selected), case
        for (index, _), value in zip(selected, actual):
            outputs[index] = value
        stderr[case] = result.stderr
    assert all(value is not None for value in outputs)
    return outputs, stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--rust-dir', type=Path)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--record-focused', action='store_true')
    parser.add_argument('--focused', action='store_true')
    args = parser.parse_args()
    source_inputs = provenance()
    cases = inputs()
    assert len(cases) == 360
    indexes = json.loads(SELECTION.read_text())['indices'] if args.focused or args.record_focused else list(range(len(cases)))
    selected = [cases[i] for i in indexes]
    path = FOCUSED if args.focused or args.record_focused else CORPUS
    if args.record or args.record_focused:
        if not args.rust_dir or args.cpp or path.exists() or (args.record and args.record_focused):
            parser.error('Record new immutable original evidence through --rust-dir only')
        expected, stderr = observe(selected, rust_dir=args.rust_dir)
        record = dict(source_inputs=source_inputs, indices=indexes,
                      baseline=json.loads((ROOT / 'upstream.lock.json').read_text()),
                      scope='A fresh process per domain with the exact retained order, actual module/schema/sample entry, full rows/reports/errors/timeless SQL. No operand/error normalization.',
                      cases=[dict(input=value, expected=result) for value, result in zip(selected, expected)], stderr=stderr)
        if args.record_focused:
            record['selection_sha256'] = digest(SELECTION)
        path.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=False).encode(), mtime=0))
        print(f'Recorded {len(selected)} original configured-type sample scenarios')
        return
    verify_record(path)
    record = json.loads(gzip.decompress(path.read_bytes()))
    assert record['source_inputs'] == source_inputs and record['indices'] == indexes
    assert record['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())
    assert same(selected, [case['input'] for case in record['cases']])
    if args.focused:
        assert record['selection_sha256'] == digest(SELECTION)
    expected = [case['expected'] for case in record['cases']]
    if args.rust_dir:
        assert same(observe(selected, rust_dir=args.rust_dir)[0], expected), 'Original outcomes changed'
    if args.cpp:
        actual = observe(selected, cpp=args.cpp)[0]
        differences = [i for i, a, b in zip(indexes, actual, expected) if not same(a, b)]
        if differences:
            failure = ROOT / 'docs/parity/sample_types_initial_comparison_5_0_6.json.gz'
            if not failure.exists():
                failure.write_bytes(gzip.compress(json.dumps(dict(indices=indexes, actual=actual, expected=expected, differences=differences), ensure_ascii=False).encode(), mtime=0))
            raise AssertionError(f'Configured-type sample differences: {differences}')
    if not args.cpp and not args.rust_dir:
        parser.error('Replay requires --cpp or --rust-dir')
    rounds = [round for value in expected for round in value.get('rounds', [])]
    print(f'Verified {len(selected)} configured-type sample scenarios, {len(rounds)} invocations and {sum(len(round["logs"]) for round in rounds)} exact ordered SQL diagnostics')


if __name__ == '__main__':
    main()
