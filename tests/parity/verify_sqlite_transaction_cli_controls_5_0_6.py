#!/usr/bin/env python3
"""Reject altered transaction source, corpus and installed CTest evidence."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from sqlite_transaction_cli_source_binding_5_0_6 import CONTRACT, Validator, read


def rejected(label, action):
    try:
        action()
    except (RuntimeError, KeyError, AssertionError, ValueError, FileNotFoundError):
        return dict(name=label, rejected=True)
    raise AssertionError('Accepted corrupted transaction evidence: ' + label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ctest-build-dir', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Use a fresh evidence path')
    contract = read(CONTRACT)
    validator = Validator(contract)
    validator.validate_files()
    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.ctest_build_dir), '--show-only=json-v1'],
        text=True))['tests']
    validator.validate_commands(tests, args.ctest_build_dir)
    controls = []

    def changed_corpus(label, name, mutate):
        candidate = copy.deepcopy(read(ROOT / contract['matrices'][name]['corpus']))
        mutate(candidate)
        controls.append(rejected(label, lambda: validator.validate_corpus(name, candidate)))

    changed_corpus('sync_changed_input', 'sqlite_transport',
                   lambda x: x['cases'][0]['input'].__setitem__('async', True))
    changed_corpus('async_changed_input', 'sqlite_async_schedule',
                   lambda x: x['cases'][0]['input'].__setitem__('async', False))
    changed_corpus('lease_changed_input', 'sqlite_async_lease',
                   lambda x: x['cases'][0].update(input={}))
    changed_corpus('sync_changed_output', 'sqlite_transport',
                   lambda x: x['cases'][0].update(expected={}))
    changed_corpus('async_missing_case', 'sqlite_async_schedule',
                   lambda x: x['cases'].pop())
    changed_corpus('lease_wrong_baseline', 'sqlite_async_lease',
                   lambda x: x.update(baseline='wrong'))

    def changed_contract(label, mutate):
        candidate = copy.deepcopy(contract)
        mutate(candidate)
        controls.append(rejected(label, lambda: Validator(candidate).validate_files()))

    changed_contract('changed_support_hash', lambda x: x['support_files'].update(
        {x['replay_script']: '0' * 64}))
    changed_contract('changed_original_receipt_hash',
                     lambda x: x['evidence_files'].update(
                         {x['original_receipt']: '0' * 64}))
    changed_contract('changed_historical_hash', lambda x: x['evidence_files'].update(
        {x['historical_corpus']: '0' * 64}))
    changed_contract('wrong_dependency_count',
                     lambda x: x.update(dependency_packages=0))
    changed_contract('wrong_runtime_commit',
                     lambda x: x.update(runtime_commit='wrong'))

    def comparison(entries):
        return next(row for row in entries if row['name'] == contract['comparison'])

    def changed_tests(label, mutate):
        candidate = copy.deepcopy(tests)
        mutate(candidate)
        controls.append(rejected(label, lambda: validator.validate_commands(
            candidate, args.ctest_build_dir)))

    changed_tests('wrong_async_probe', lambda x: comparison(x)['command']
                  .__setitem__(-2, '/tmp/wrong-probe'))
    changed_tests('missing_source_fixture',
                  lambda x: comparison(x).update(properties=[]))
    changed_tests('duplicate_comparison',
                  lambda x: x.append(copy.deepcopy(comparison(x))))
    changed_tests('missing_comparison',
                  lambda x: x.remove(comparison(x)))

    assert len(controls) == 15 and all(row['rejected'] for row in controls)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=controls),
            indent=2) + '\n').encode(), mtime=0))
    print('Rejected 15 transaction source/CTest substitutions')


if __name__ == '__main__':
    main()
