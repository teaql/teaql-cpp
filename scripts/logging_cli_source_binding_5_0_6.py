#!/usr/bin/env python3
"""Bind installed log-manager and audit-event comparisons to Rust 5.0.6 source."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/logging_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_log_formatter import count_fits
from compare_log_manager import observe as observe_log
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
                'locked runtime commit')
        reference = ROOT / '.reference/teaql-rs'
        head = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                       text=True).strip()
        require(head == contract['runtime_commit'], 'original source checkout')
        dirty = subprocess.check_output(['git', '-C', str(reference), 'status',
                                         '--porcelain', '--untracked-files=no'], text=True)
        require(not dirty, 'unchanged original source checkout')
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for path, wanted in contract['evidence_files'].items():
            require(digest(ROOT / path) == wanted, 'retained evidence: ' + path)
        manifest = ROOT / contract['source_manifest']
        lock = manifest.with_name('Cargo.lock')
        reference_lock = reference / 'Cargo.lock'
        require(registry_identities(lock) <= registry_identities(reference_lock),
                'reference-seeded registry identities')
        reference_versions = versions(reference_lock)
        for package, values in versions(lock).items():
            if package != 'teaql_cpp_logging_source_506':
                require(values <= reference_versions.get(package, set()),
                        'reference-seeded dependency version: ' + package)
        for name, row in contract['comparisons'].items():
            corpus = read(ROOT / row['corpus'])
            original = read(ROOT / row['original_receipt'])
            require(original['complete'] is True and original['whole_goal_complete'] is False
                    and original['runtime_commit'] == contract['runtime_commit']
                    and original['build_exit_code'] == 0
                    and original['comparison_exit_code'] == 0
                    and original['count'] == row['count']
                    and original['temporary_target_removed'] is True,
                    'fresh original source execution: ' + name)
            for key, path in (('source_manifest_sha256', contract['source_manifest']),
                              ('source_lock_sha256', contract['source_lock']),
                              ('source_driver_sha256', row['original_driver'])):
                require(original[key] == digest(ROOT / path),
                        'original source input: ' + path)
            require(original['reference_lock_sha256'] == digest(reference_lock),
                    'exact reference dependency lock')
            require(original['log_normalization_applied'] == (name == 'log_manager'),
                    'documented log normalization only')
            require(len(original['original_outputs']) == row['count']
                    and same(original['original_outputs'],
                             [case['expected'] for case in corpus['cases']]),
                    'complete fresh original outputs: ' + name)

    def validate_corpus(self, name, candidate=None):
        row = self.contract['comparisons'][name]
        retained = read(ROOT / row['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        historical = read(ROOT / 'tests/parity' / corpus['historical_corpus'])
        require(corpus['baseline'] == self.contract['runtime_commit']
                and corpus['historical_corpus'] == row['historical_corpus']
                and corpus['historical_sha256'] == digest(ROOT / 'tests/parity' /
                                                         row['historical_corpus'])
                and corpus['original_driver'] == row['original_driver']
                and corpus['original_driver_sha256'] == digest(ROOT / row['original_driver'])
                and corpus['reference_lock_sha256']
                == digest(ROOT / '.reference/teaql-rs/Cargo.lock')
                and type(corpus['cases']) is list and len(corpus['cases']) == row['count'],
                'source corpus metadata: ' + name)
        require(len(historical['cases']) == row['count']
                and same([case['input'] for case in corpus['cases']],
                         [case['input'] for case in historical['cases']])
                and same(corpus['cases'], retained['cases']),
                'unchanged complete source corpus: ' + name)
        return corpus

    def validate_commands(self, tests, build):
        for name, row in self.contract['comparisons'].items():
            script = str(ROOT / row['comparator'])
            selected = [test for test in tests
                        if len(test.get('command', [])) > 1
                        and test['command'][1] == script]
            require(len(selected) == 1 and selected[0]['name'] == row['comparison'],
                    'unique installed comparison: ' + name)
            test = selected[0]
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [script, '--cpp',
                                               str(build.resolve() / row['probe']),
                                               '--cpp-emulator='],
                    'exact installed comparison command: ' + name)
            props = {item['name']: item['value'] for item in test.get('properties', [])}
            require('logging_cli' in props.get('FIXTURES_REQUIRED', []),
                    'source fixture on installed comparison: ' + name)

    def validate_ctest(self, build):
        tests = read_ctest(build)
        self.validate_commands(tests, build)

    def validate_native(self, build):
        result = {}
        for name, row in self.contract['comparisons'].items():
            corpus = self.validate_corpus(name)
            inputs = [case['input'] for case in corpus['cases']]
            expected = [case['expected'] for case in corpus['cases']]
            binary = str(build.resolve() / row['probe'])
            if name == 'log_manager':
                actual = observe_log(binary, inputs)
                require(len(actual) == row['count'] and same(actual, expected),
                        'fresh complete log-manager outcome')
                result[name] = dict(observations=len(actual), exact_equalities=len(actual),
                                    normalization='Human UTC timestamp; Rust header correction',
                                    binary_sha256=digest(binary))
                continue
            bits = int(subprocess.check_output([binary, '--pointer-bits'], text=True))
            require(bits in (32, 64), 'supported audit target pointer width')
            accepted = [count_fits(value, bits) for value in inputs]
            count_indices = [index for index, value in enumerate(inputs)
                             if value.get('kind') in ('schema_created', 'schema_verified',
                                                     'data_seeded')]
            count_inputs = [inputs[index] for index in count_indices]
            count_expected = [dict(accepted=accepted[index]) for index in count_indices]
            require(same(observe(binary, count_inputs, arguments=['--validate-counts']),
                         count_expected), 'audit target-width count rejection')
            valid = [index for index, fits in enumerate(accepted) if fits]
            actual = observe(binary, [inputs[index] for index in valid])
            require(len(actual) == len(valid) and same(actual,
                                                      [expected[index] for index in valid]),
                    'fresh complete audit-event outcome')
            result[name] = dict(observations=len(valid), exact_equalities=len(valid),
                                target_width_rejections=len(inputs) - len(valid),
                                pointer_bits=bits, binary_sha256=digest(binary))
        return result


def read_ctest(build):
    return json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']


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
    for name in validator.contract['comparisons']:
        validator.validate_corpus(name)
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        receipt = dict(complete=True, whole_goal_complete=False, native=native,
                       fresh_original_observations={name: row['count'] for name, row
                                                    in validator.contract['comparisons'].items()},
                       contract_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Verified original logging sources: 94 log-manager and 1,427 audit-event outcomes'
          + (' / fresh C++ results' if native else ''))


if __name__ == '__main__':
    main()
