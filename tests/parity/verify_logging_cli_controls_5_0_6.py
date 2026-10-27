#!/usr/bin/env python3
"""Reject substituted logging source, complete output, or CTest evidence."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from logging_cli_source_binding_5_0_6 import CONTRACT, Validator, read
from compare_log_manager import AUDIT_HELP_LINE, CPP_HEADER, RUST_HEADER, normalize


def rejected(name, action):
    try:
        action()
    except (RuntimeError, KeyError, AssertionError, ValueError, FileNotFoundError):
        return dict(name=name, rejected=True)
    raise AssertionError('Accepted corrupted logging evidence: ' + name)


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
    for name in contract['comparisons']:
        validator.validate_corpus(name)
    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.ctest_build_dir), '--show-only=json-v1'],
        text=True))['tests']
    validator.validate_commands(tests, args.ctest_build_dir)
    controls = []

    def changed_corpus(label, name, mutate):
        candidate = copy.deepcopy(read(ROOT / contract['comparisons'][name]['corpus']))
        mutate(candidate)
        controls.append(rejected(label, lambda: validator.validate_corpus(name, candidate)))

    changed_corpus('log_changed_input', 'log_manager',
                   lambda x: x['cases'][0]['input'].update(kind='wrong'))
    changed_corpus('audit_changed_input', 'audit_event',
                   lambda x: x['cases'][0]['input'].update(kind='wrong'))
    changed_corpus('log_changed_output', 'log_manager',
                   lambda x: x['cases'][0].update(expected={}))
    changed_corpus('audit_changed_output', 'audit_event',
                   lambda x: x['cases'][0].update(expected={}))
    changed_corpus('log_missing_case', 'log_manager', lambda x: x['cases'].pop())
    changed_corpus('audit_wrong_baseline', 'audit_event',
                   lambda x: x.update(baseline='wrong'))
    changed_corpus('log_wrong_driver', 'log_manager',
                   lambda x: x.update(original_driver_sha256='0' * 64))

    def changed_contract(label, mutate):
        candidate = copy.deepcopy(contract)
        mutate(candidate)
        controls.append(rejected(label, lambda: Validator(candidate).validate_files()))

    changed_contract('changed_support_hash', lambda x: x['support_files'].update(
        {x['source_manifest']: '0' * 64}))
    changed_contract('changed_log_receipt_hash', lambda x: x['evidence_files'].update(
        {x['comparisons']['log_manager']['original_receipt']: '0' * 64}))
    changed_contract('changed_audit_receipt_hash', lambda x: x['evidence_files'].update(
        {x['comparisons']['audit_event']['original_receipt']: '0' * 64}))
    changed_contract('wrong_runtime_commit', lambda x: x.update(runtime_commit='wrong'))

    def comparison(entries, name):
        wanted = contract['comparisons'][name]['comparison']
        return next(row for row in entries if row['name'] == wanted)

    def changed_tests(label, mutate):
        candidate = copy.deepcopy(tests)
        mutate(candidate)
        controls.append(rejected(label, lambda: validator.validate_commands(
            candidate, args.ctest_build_dir)))

    changed_tests('wrong_log_probe', lambda x: comparison(x, 'log_manager')['command']
                  .__setitem__(-2, '/tmp/wrong-probe'))
    changed_tests('missing_log_fixture', lambda x: comparison(x, 'log_manager')
                  .update(properties=[]))
    changed_tests('duplicate_audit_comparison', lambda x: x.append(copy.deepcopy(
        comparison(x, 'audit_event'))))
    changed_tests('missing_audit_comparison', lambda x: x.remove(
        comparison(x, 'audit_event')))

    assert len(controls) == 15 and all(row['rejected'] for row in controls)
    banner = AUDIT_HELP_LINE + '\n' + RUST_HEADER + '\n\n[Extreme Performance Testing: Disable Logs]\n'
    ordinary = 'application text: ' + RUST_HEADER + '\n'
    copied_section = 'application text:\n' + RUST_HEADER + '\n\n[Extreme Performance Testing: Disable Logs]\n'
    assert normalize(banner) == banner.replace(RUST_HEADER, CPP_HEADER)
    assert normalize(ordinary) == ordinary
    assert normalize(copied_section) == copied_section
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=controls,
            normalization_scope=dict(banner_corrected=True, ordinary_text_preserved=True,
                                     copied_section_preserved=True)),
            indent=2) + '\n').encode(), mtime=0))
    print('Rejected 15 logging source/CTest substitutions; verified banner-only normalization')


if __name__ == '__main__':
    main()
