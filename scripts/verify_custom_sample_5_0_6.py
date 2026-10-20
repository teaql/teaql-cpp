#!/usr/bin/env python3
"""Record/replay actual customized sample rows, reports and timeless SQL diagnostics."""
import argparse
import copy
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

OUTPUT = ROOT / 'tests/parity/custom_sample_5_0_6_cases.json.gz'
FOCUSED_OUTPUT = ROOT / 'tests/parity/custom_sample_5_0_6_focused_cases.json.gz'
SELECTION = ROOT / 'tests/parity/custom_sample_5_0_6_instrumented_selection.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    modules = ('plain', 'checker', 'behavior', 'combined')
    def case(module, checker, behavior, **extra):
        return dict(dict(fixture='candidates', module=module, checker=checker, behavior=behavior,
                         state='normal', seed=7, rounds=1, scale='Tiny', governance='default', enabled=True), **extra)
    cases = [case(module, checker, behavior)
             for module, checker, behavior in itertools.product(modules,
                 ('none', 'fix_person', 'reject_person', 'reject_record'),
                 ('none', 'record', 'reverse_constants', 'deny_query', 'deny_insert_person', 'deny_insert_record'))]
    cases += [case(module, 'fix_person', 'record', governance=policy, rounds=2)
              for module, policy in itertools.product(modules, ('allow', 'approved', 'deny_record'))]
    cases += [case(module, 'fix_person', 'record', state=state, rounds=2)
              for module, state in itertools.product(modules, ('missing_root', 'missing_constant'))]
    cases += [case(module, 'fix_person', 'record', fault=target, rounds=2)
              for module, target in itertools.product(modules, ('person', 'record'))]
    cases += [case(module, checker, 'record', enabled=False)
              for module, checker in itertools.product(modules, ('none', 'fix_person'))]
    cases += [case(module, checker, 'reverse_constants', rounds=2)
              for module, checker in itertools.product(modules, ('fix_person', 'reject_record'))]
    return cases


def provenance():
    source_inputs = {}
    support = json.loads((ROOT / 'tests/parity/custom_sample_5_0_6_support.json').read_text())
    for path, expected in support['files'].items():
        assert digest(ROOT / path) == expected, path
        source_inputs[path] = expected
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())
    for fixture in ('candidates',):
        directory = ROOT / f'tests/generator/parity/{fixture}_rust'
        record = json.loads((directory / 'PROVENANCE.json').read_text())
        assert record['hand_edited'] is False
        assert record['runtime_commit'] == baseline['commit']
        assert record['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
        for path, expected in record['files'].items():
            assert digest(directory / path) == expected, path
        assert digest(ROOT / record['model']) == record['model_sha256']
        for path in [directory / 'PROVENANCE.json', directory / 'lib/src/sample_data.rs']:
            source_inputs[str(path.relative_to(ROOT))] = digest(path)
    project = ROOT / 'tests/parity/rust_custom_sample_5_0_6'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    for name in ['Cargo.toml', 'Cargo.lock', 'src/main.rs']:
        path = project / name
        source_inputs[str(path.relative_to(ROOT))] = digest(path)
    return source_inputs


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


def observe_original(executable, cases):
    raw, stderr = observe(executable, cases)
    expected = copy.deepcopy(raw)
    for output in expected:
        for round in output.get('rounds', []):
            records = round.pop('source_records')
            for record in records:
                assert set(record) == {'level', 'target', 'message'}
                assert record['level'] == 'INFO' and record['target'] == 'cpp_candidates_core::sample_data'
            assert [record['message'] for record in records] == round['messages']
    return expected, stderr, raw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--record-focused', action='store_true')
    parser.add_argument('--cases', type=int, nargs='+', help='Use the exact recorded instrumented selection and its independent original corpus')
    args = parser.parse_args()
    source_inputs = provenance()
    cases = inputs()
    assert len(cases) == 140
    if args.record_focused:
        if not args.rust or args.cpp or args.record or args.cases or FOCUSED_OUTPUT.exists():
            parser.error('Record focused original evidence only through --rust into a new corpus')
        indexes = json.loads(SELECTION.read_text())['indices']
        selected = [cases[index] for index in indexes]
        expected, stderr, raw = observe_original(args.rust, selected)
        FOCUSED_OUTPUT.write_bytes(gzip.compress(json.dumps(dict(
            source_inputs=source_inputs, source_raw=raw, selection_sha256=digest(SELECTION), indices=indexes,
            scope='Actual original execution of this ordered subset in one fresh process; process-wide mutation execution IDs are not rewritten.',
            cases=[dict(input=value, expected=result) for value, result in zip(selected, expected)],
            stderr=stderr), ensure_ascii=False).encode(), mtime=0))
        print(f'Recorded {len(indexes)} independent focused original sample scenarios')
        return
    if args.record:
        if not args.rust or args.cpp or args.cases or OUTPUT.exists():
            parser.error('Record all cases through --rust into new immutable evidence only')
        expected, stderr, raw = observe_original(args.rust, cases)
        OUTPUT.write_bytes(gzip.compress(json.dumps(dict(
            baseline=json.loads((ROOT / 'upstream.lock.json').read_text()),
            source_inputs=source_inputs, source_raw=raw,
            scope='Actual generated module/schema/sample entry. Complete rows/reports/SQL/trace/error outcomes; only wall-clock timing fields omitted as in the established sample contract.',
            cases=[dict(input=input_value, expected=value) for input_value, value in zip(cases, expected)],
            stderr=stderr), ensure_ascii=False).encode(), mtime=0))
        print(f'Recorded {len(cases)} complete original customized sample scenarios')
        return
    record = json.loads(gzip.decompress(OUTPUT.read_bytes()))
    assert record['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())
    assert record['source_inputs'] == source_inputs
    assert same(cases, [case['input'] for case in record['cases']])
    indexes = args.cases or list(range(len(cases)))
    assert len(set(indexes)) == len(indexes) and all(0 <= index < len(cases) for index in indexes)
    selected = [cases[index] for index in indexes]
    expected = [record['cases'][index]['expected'] for index in indexes]
    if args.cases:
        focused = json.loads(gzip.decompress(FOCUSED_OUTPUT.read_bytes()))
        assert focused['source_inputs'] == source_inputs
        assert focused['selection_sha256'] == digest(SELECTION)
        assert focused['indices'] == indexes, 'Use the exact retained instrumented order'
        assert same(selected, [case['input'] for case in focused['cases']])
        expected = [case['expected'] for case in focused['cases']]
    if args.rust:
        assert same(observe_original(args.rust, selected)[0], expected), 'Original sample outcomes changed'
    if args.cpp:
        actual = observe(args.cpp, selected)[0]
        differences = [index for index, left, right in zip(indexes, actual, expected) if not same(left, right)]
        if differences:
            path = ROOT / 'docs/parity/custom_sample_initial_comparison_5_0_6.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress(json.dumps(dict(indexes=indexes, actual=actual,
                    expected=expected, differences=differences), ensure_ascii=False).encode(), mtime=0))
            raise AssertionError(f'customized sample differences: {differences}; raw outcomes retained')
    if not args.cpp and not args.rust:
        parser.error('Replay requires --cpp or --rust')
    rounds = sum(len(value.get('rounds', [])) for value in expected)
    logs = sum(len(round['logs']) for value in expected for round in value.get('rounds', []))
    plans = sum(len(round["plans"]) for value in expected for round in value.get("rounds", []))
    calls = sum(len(round['calls']) for value in expected for round in value.get('rounds', []))
    messages = sum(len(round['messages']) for value in expected for round in value.get('rounds', []))
    print(f'Verified {len(indexes)} customized sample scenarios, {rounds} invocations, {plans} policy plans, {calls} callbacks, {messages} Info messages and {logs} ordered SQL records')


if __name__ == '__main__':
    main()
