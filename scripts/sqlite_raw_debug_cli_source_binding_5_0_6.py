#!/usr/bin/env python3
"""Bind installed SQLite raw FromSql Debug comparison to locked Rust source."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/sqlite_raw_debug_cli_contract_5_0_6.json'
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
        manifest = ROOT / contract['source_manifest']
        lock = ROOT / contract['source_lock']
        reference_lock = reference / 'Cargo.lock'
        require(registry_identities(lock) <= registry_identities(reference_lock),
                'reference-seeded registry identities')
        wanted_versions = versions(reference_lock)
        resolved = versions(lock)
        require(len(resolved) == contract['dependency_packages'],
                'complete narrow source closure')
        for package, values in resolved.items():
            if package != 'teaql_cpp_sqlite_raw_debug_source_506':
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded dependency version: ' + package)
        original = read(ROOT / contract['original_receipt'])
        require(original['complete'] is True and original['whole_goal_complete'] is False
                and original['runtime_commit'] == contract['runtime_commit']
                and original['build_exit_code'] == 0
                and original['comparison_exit_code'] == 0
                and original['count'] == contract['count']
                and original['temporary_target_removed'] is True,
                'fresh original execution receipt')
        for field, path in (('source_manifest_sha256', manifest),
                            ('source_lock_sha256', lock),
                            ('reference_lock_sha256', reference_lock),
                            ('source_driver_sha256', ROOT / contract['driver'])):
            require(original[field] == digest(path),
                    'original source input: ' + str(path))
        expected = [case['expected'] for case in self.validate_corpus()['cases']]
        require(len(original['original_outputs']) == contract['count']
                and same(original['original_outputs'], expected),
                'complete fresh original raw Debug outputs')

    def validate_corpus(self, candidate=None):
        contract = self.contract
        retained = read(ROOT / contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        historical = read(ROOT / 'tests/parity' / contract['historical_corpus'])
        require(corpus['baseline'] == contract['runtime_commit']
                and corpus['checked_arithmetic'] is True
                and corpus['historical_corpus'] == contract['historical_corpus']
                and corpus['historical_corpus_sha256']
                == digest(ROOT / 'tests/parity' / contract['historical_corpus'])
                and corpus['original_driver'] == contract['driver']
                and corpus['original_driver_sha256'] == digest(ROOT / contract['driver'])
                and type(corpus['cases']) is list
                and len(corpus['cases']) == contract['count'],
                'complete raw Debug corpus metadata')
        require(len(historical['cases']) == contract['count']
                and same([case['input'] for case in corpus['cases']],
                         [case['input'] for case in historical['cases']])
                and same(corpus['cases'], retained['cases']),
                'unchanged historical inputs and frozen source outcomes')
        return corpus

    def validate_commands(self, tests, build):
        contract = self.contract
        script = str(ROOT / contract['comparator'])
        selected = [test for test in tests
                    if len(test.get('command', [])) > 1
                    and test['command'][1] == script]
        require(len(selected) == 1 and selected[0]['name'] == contract['comparison'],
                'unique installed raw Debug comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / contract['probe'])],
                'exact installed raw Debug comparison command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('sqlite_raw_debug_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed raw Debug comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        corpus = self.validate_corpus()
        inputs = [case['input'] for case in corpus['cases']]
        expected = [case['expected'] for case in corpus['cases']]
        binary = str(build.resolve() / self.contract['probe'])
        actual = observe(binary, inputs, arguments=['--raw-debug'])
        require(len(actual) == self.contract['count'] and same(actual, expected),
                'fresh complete native raw Debug results')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    normalization='none', binary_sha256=digest(binary))


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
    validator.validate_corpus()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      fresh_original_observations=validator.contract['count'],
                      native=native, contract_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(), mtime=0))
    print('Verified original SQLite raw Debug: 36 Rust outcomes'
          + (' / 36 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
