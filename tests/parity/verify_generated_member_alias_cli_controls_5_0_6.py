#!/usr/bin/env python3
"""Exercise negative source and CTest controls for generated member aliases."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from generated_member_alias_cli_source_binding_5_0_6 import (
    CONTRACT, Validator, read)


def rejected(name, action):
    try:
        action()
    except (RuntimeError, KeyError, AssertionError, ValueError, FileNotFoundError):
        return dict(name=name, rejected=True)
    raise AssertionError('Accepted corrupted member-alias evidence: ' + name)


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

    changed_corpus('changed_input', lambda x: x['inputs'][0].update(model='holder'))
    changed_corpus('swapped_input_order', lambda x: x['inputs'].__setitem__(
        slice(0, 2), list(reversed(x['inputs'][:2]))))
    changed_corpus('changed_original_value', lambda x: x['expected'][2].update(value='wrong'))
    changed_corpus('missing_expected', lambda x: x['expected'].pop())
    changed_corpus('missing_input', lambda x: x['inputs'].pop())
    changed_corpus('wrong_runtime', lambda x: x.update(runtime_commit='wrong'))
    changed_corpus('missing_loaded_field', lambda x: x['expected'][0]['loaded'].pop('id'))

    def changed_contract(name, mutate):
        candidate = copy.deepcopy(registry)
        mutate(candidate)
        controls.append(rejected(name, lambda: Validator(candidate).validate_files()))

    changed_contract('changed_support_hash', lambda x: x['support_files'].update(
        {x['comparator']: '0' * 64}))
    changed_contract('changed_corpus_hash', lambda x: x.update(corpus_sha256='0' * 64))
    changed_contract('changed_original_receipt_hash',
                     lambda x: x.update(source_receipt_sha256='0' * 64))
    changed_contract('changed_generated_file_count',
                     lambda x: x.update(generated_files=0))

    def changed_tests(name, mutate):
        candidate = copy.deepcopy(tests)
        mutate(candidate)
        controls.append(rejected(name, lambda: validator.validate_commands(
            candidate, args.ctest_build_dir)))

    test_name = registry['comparison']

    def comparison(entries):
        return next(row for row in entries if row['name'] == test_name)

    changed_tests('wrong_comparison_binary', lambda x: comparison(x)['command'].__setitem__(
        -1, '/tmp/wrong-probe'))
    changed_tests('missing_source_fixture', lambda x: comparison(x).update(properties=[
        p for p in comparison(x)['properties'] if p['name'] != 'FIXTURES_REQUIRED']))
    changed_tests('duplicate_source_comparison', lambda x: x.append(copy.deepcopy(comparison(x))))
    changed_tests('missing_source_comparison', lambda x: x.remove(comparison(x)))

    assert len(controls) == 15 and all(row['rejected'] for row in controls)
    if args.record:
        receipt = dict(complete=True, whole_goal_complete=False,
                       controls=controls)
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Rejected 15 generated member-alias source/CTest substitutions')


if __name__ == '__main__':
    main()
