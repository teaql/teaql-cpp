#!/usr/bin/env python3
"""Bind installed SQLite transaction comparison to original Rust 5.0.6 replay."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/sqlite_transaction_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_values import observe, same
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def read(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


class Validator:
    def __init__(self, contract):
        self.contract = contract

    def validate_files(self):
        contract = self.contract
        require(read(ROOT / 'upstream.lock.json')['commit'] == contract['runtime_commit'],
                'locked runtime')
        reference = ROOT / '.reference/teaql-rs'
        head = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                       text=True).strip()
        dirty = subprocess.check_output(['git', '-C', str(reference), 'status',
                                         '--porcelain', '--untracked-files=no'], text=True)
        require(head == contract['runtime_commit'] and not dirty,
                'unchanged original source checkout')
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for path, wanted in contract['evidence_files'].items():
            require(digest(ROOT / path) == wanted, 'retained evidence: ' + path)
        reference_lock = reference / 'Cargo.lock'
        observer_lock = ROOT / contract['observer_lock']
        require(registry_identities(observer_lock) <= registry_identities(reference_lock),
                'reference-seeded registry identities')
        resolved = versions(observer_lock)
        require(len(resolved) == contract['dependency_packages'],
                'complete transaction observer closure')
        wanted_versions = versions(reference_lock)
        for package, values in resolved.items():
            if package != 'teaql_sqlite_transaction_streaming_reference_506':
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded dependency version: ' + package)
        identities = read(ROOT / contract['observer_manifest'])
        require(identities['baseline'] == contract['runtime_commit'],
                'source observer manifest baseline')
        for path, wanted in identities['files'].items():
            require(digest(ROOT / path) == wanted, 'unchanged observer/corpus: ' + path)
        original = read(ROOT / contract['original_receipt'])
        require(original['complete'] is True
                and original['full_goal_accepted'] is False
                and original['baseline'] == contract['runtime_commit']
                and original['reference_lock_sha256'] == digest(reference_lock)
                and original['observer_lock_sha256'] == digest(observer_lock)
                and original['verifier_sha256'] == digest(ROOT / contract['replay_script'])
                and same(original['observer_manifest'], identities)
                and original['frontends'] == ['native']
                and original['cases_per_frontend'] == contract['total_count'],
                'fresh complete original transaction replay')
        steps = original['steps']
        require(any(step['command'][:3] == ['cargo', 'build', '--offline']
                    for step in steps)
                and all(step['exit_code'] == 0 for step in steps),
                'successful original source build and execution')
        require(len(original['matrices']) == len(contract['matrices']),
                'complete original transaction matrices')
        for row in original['matrices']:
            name = row['name']
            require(name in contract['matrices'] and row['cases']
                    == contract['matrices'][name]['count'],
                    'original transaction family: ' + name)
            corpus = self.validate_corpus(name)
            expected = [case['expected'] for case in corpus['cases']]
            require(row['corpus_sha256'] == digest(ROOT / contract['matrices'][name]['corpus'])
                    and len(row['original']) == row['cases']
                    and same(row['original'], expected)
                    and set(row['frontends']) == {'native'}
                    and same(row['frontends']['native']['outcomes'], expected),
                    'complete original/native outcomes: ' + name)

    def validate_corpus(self, name, candidate=None):
        row = self.contract['matrices'][name]
        retained = read(ROOT / row['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['baseline'] == self.contract['runtime_commit']
                and len(corpus['cases']) == row['count']
                and same(corpus['cases'], retained['cases']),
                'unchanged transaction source corpus: ' + name)
        if name != 'sqlite_async_lease':
            historical = read(ROOT / self.contract['historical_corpus'])
            asynchronous = name == 'sqlite_async_schedule'
            inputs = [case['input'] for case in historical['cases']
                      if case['input']['async'] == asynchronous]
            require(len(inputs) == row['count']
                    and same([case['input'] for case in corpus['cases']], inputs),
                    'complete historical transaction inputs: ' + name)
        return corpus

    def validate_commands(self, tests, build):
        contract = self.contract
        script = str(ROOT / contract['comparator'])
        selected = [test for test in tests
                    if len(test.get('command', [])) > 1
                    and test['command'][1] == script]
        require(len(selected) == 1 and selected[0]['name'] == contract['comparison'],
                'unique installed transaction comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [
                    script, '--cpp', str(build.resolve() / 'teaql_sqlite_transport_probe'),
                    '--cpp-async',
                    str(build.resolve() / 'teaql_sqlite_async_schedule_probe'),
                    '--cpp-emulator='],
                'exact installed transaction comparison command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('sqlite_transaction_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed transaction comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        results = {}
        for name, row in self.contract['matrices'].items():
            corpus = self.validate_corpus(name)
            inputs = [case['input'] for case in corpus['cases']]
            expected = [case['expected'] for case in corpus['cases']]
            binary = str(build.resolve() / row['probe'])
            actual = observe(binary, inputs)
            require(len(actual) == row['count'] and same(actual, expected),
                    'fresh complete native transaction results: ' + name)
            results[name] = dict(observations=len(actual), exact_equalities=len(actual),
                                 binary_sha256=digest(binary))
        return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, default=CONTRACT)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--ctest-build-dir', type=Path)
    parser.add_argument('--cpp-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.verify_only or args.cpp_build_dir is not None,
            'select source or current native validation')
    require(args.record is None or not args.record.exists(),
            'preserve historical evidence')
    validator = Validator(read(args.contracts))
    validator.validate_files()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      fresh_original_observations=validator.contract['total_count'],
                      native=native, contract_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(), mtime=0))
    print('Verified original SQLite transactions: 510 Rust outcomes'
          + (' / 510 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
