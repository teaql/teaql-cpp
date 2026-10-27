#!/usr/bin/env python3
"""Reject corrupted inherited Business-ID bridge and CTest evidence."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from inherited_business_cli_source_binding_5_0_6 import CONTRACT, Validator, read


def rejected(name, action):
    try:
        action()
    except (RuntimeError, KeyError, AssertionError, ValueError, FileNotFoundError):
        return dict(name=name, rejected=True)
    raise AssertionError('Accepted corrupted inherited business evidence: ' + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ctest-build-dir', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Use a fresh evidence path')
    registry = read(CONTRACT)
    validator = Validator(registry)
    validator.validate_files()
    validator.validate_corpus()
    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.ctest_build_dir), '--show-only=json-v1'],
        text=True))['tests']
    validator.validate_commands(tests, args.ctest_build_dir)
    corpus = read(ROOT / registry['corpus'])
    controls = []

    def changed_corpus(name, mutate):
        candidate = copy.deepcopy(corpus)
        mutate(candidate)
        controls.append(rejected(name, lambda: validator.validate_corpus(candidate)))

    changed_corpus('wrong_model', lambda x: x['cases'][0]['input'].update(model='other'))
    changed_corpus('wrong_custom_logic',
                   lambda x: x['cases'][0]['input'].update(logic='other'))
    changed_corpus('wrong_operation',
                   lambda x: x['cases'][0]['input'].update(operation='delete'))
    changed_corpus('changed_policy_result',
                   lambda x: x['cases'][0]['expected'].update(error='wrong'))
    changed_corpus('missing_case', lambda x: x['cases'].pop())
    changed_corpus('wrong_baseline', lambda x: x.update(baseline='wrong'))
    changed_corpus('false_untouched_acceptance',
                   lambda x: x.update(source_artifact_acceptance=True))

    def changed_contract(name, mutate):
        candidate = copy.deepcopy(registry)
        mutate(candidate)
        controls.append(rejected(name, lambda: Validator(candidate).validate_files()))

    changed_contract('changed_support_hash', lambda x: x['support_files'].update(
        {x['comparator']: '0' * 64}))
    changed_contract('changed_corpus_hash', lambda x: x.update(corpus_sha256='0' * 64))
    changed_contract('changed_original_execution_hash',
                     lambda x: x.update(original_receipt_sha256='0' * 64))
    changed_contract('wrong_bridge_file_count',
                     lambda x: x.update(generated_files=0))

    name = registry['comparison']

    def comparison(entries):
        return next(row for row in entries if row['name'] == name)

    def changed_tests(label, mutate):
        candidate = copy.deepcopy(tests)
        mutate(candidate)
        controls.append(rejected(label, lambda: validator.validate_commands(
            candidate, args.ctest_build_dir)))

    changed_tests('wrong_observer_binary', lambda x: comparison(x)['command'].__setitem__(
        -1, '/tmp/wrong-probe'))
    changed_tests('missing_source_fixture', lambda x: comparison(x).update(properties=[
        p for p in comparison(x)['properties'] if p['name'] != 'FIXTURES_REQUIRED']))
    changed_tests('duplicate_comparison', lambda x: x.append(copy.deepcopy(comparison(x))))
    changed_tests('missing_comparison', lambda x: x.remove(comparison(x)))

    assert len(controls) == 15 and all(row['rejected'] for row in controls)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=controls),
            indent=2) + '\n').encode(), mtime=0))
    print('Rejected 15 inherited business source/CTest substitutions')


if __name__ == '__main__':
    main()
