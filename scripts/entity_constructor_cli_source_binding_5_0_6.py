#!/usr/bin/env python3
"""Bind the installed constructor helper CLI to fresh original Rust evidence."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/entity_constructor_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_entity_constructor_helpers_5_0_6 import cases
from compare_values import same
from sample_clock_driver import observe_fixed
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
                'locked Rust runtime')
        for name, wanted in (('teaql-rs', contract['runtime_commit']),
                             ('teaql_code_gen', contract['generator_commit'])):
            actual = subprocess.check_output(
                ['git', '-C', str(ROOT / '.reference' / name), 'rev-parse', 'HEAD'],
                text=True).strip()
            require(actual == wanted, 'locked original checkout: ' + name)
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus', 'original_receipt', 'historical_terminal'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)

        terminal = read(ROOT / contract['historical_terminal'])
        require(terminal['runtime_commit'] == contract['runtime_commit']
                and terminal['generator_commit'] == contract['generator_commit']
                and terminal['observations'] == contract['count']
                and terminal['normalization'] == 'none'
                and terminal['source_registry_versions_preserved'] is True
                and terminal['source_registry_checksums_preserved'] is True,
                'historical original constructor scope')
        stale = [path for path, wanted in terminal['hashes'].items()
                 if digest(ROOT / path) != wanted]
        require(stale == [contract['historical_comparator_path']],
                'single historical comparator split boundary')

        provenance_path = ROOT / contract['provenance']
        provenance = read(provenance_path)
        require(provenance['runtime_commit'] == contract['runtime_commit']
                and provenance['generator_commit'] == contract['generator_commit']
                and provenance['hand_edited'] is False
                and len(provenance['files']) == contract['generated_files']
                and digest(provenance_path)
                == terminal['original_generated_entity_provenance_sha256'],
                'complete original generated inheritance domain')
        require(digest(ROOT / provenance['model']) == provenance['model_sha256'],
                'original inheritance model')
        for path, wanted in provenance['files'].items():
            require(digest(provenance_path.parent / path) == wanted,
                    'original generated file: ' + path)

        observer = ROOT / 'tests/parity/rust_entity_constructor_helpers_5_0_6/Cargo.lock'
        reference = ROOT / '.reference/teaql-rs/Cargo.lock'
        require(registry_identities(observer) <= registry_identities(reference),
                'reference-seeded observer registry closure')
        wanted_versions = versions(reference)
        for package, values in versions(observer).items():
            if package != 'teaql_entity_constructor_helpers_506':
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded observer version: ' + package)

        original = read(ROOT / contract['original_receipt'])
        require(original['complete'] is True
                and original['runtime_commit'] == contract['runtime_commit']
                and original['generator_commit'] == contract['generator_commit']
                and original['build_exit_code'] == 0
                and original['comparison_exit_code'] == 0
                and original['count'] == contract['count']
                and original['temporary_target_removed'] is True,
                'fresh original Rust execution receipt')
        for recorded, source in (
                ('source_manifest_sha256',
                 'tests/parity/rust_entity_constructor_helpers_5_0_6/Cargo.toml'),
                ('source_lock_sha256',
                 'tests/parity/rust_entity_constructor_helpers_5_0_6/Cargo.lock'),
                ('source_observer_sha256',
                 'tests/parity/entity_constructor_helpers_5_0_6.rs'),
                ('original_generated_provenance_sha256', contract['provenance'])):
            require(original[recorded] == digest(ROOT / source),
                    'fresh original source: ' + source)
        require(original['clock_sha256']
                == digest(ROOT / 'build_native_current' / contract['clock_binary']),
                'same controlled wall clock')
        corpus = read(ROOT / contract['corpus'])
        require(same(original['original_outputs'],
                     [row['expected'] for row in corpus['cases']]),
                'complete fresh original Rust outcomes')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['baseline'] == self.contract['runtime_commit']
                and len(corpus['cases']) == self.contract['count'],
                'complete original constructor corpus')
        require(same([row['input'] for row in corpus['cases']], list(cases())),
                'complete original constructor input matrix')
        require(same([row['expected'] for row in corpus['cases']],
                     [row['expected'] for row in retained['cases']]),
                'unchanged complete constructor outcomes')
        require({row['input']['model'] for row in corpus['cases']}
                == {'document', 'intermediate_record', 'invoice', 'receipt',
                    'special_invoice'},
                'all original inheritance models')
        return corpus

    def validate_commands(self, tests, build):
        script = str(ROOT / self.contract['comparator'])
        selected = [row for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script]
        require(len(selected) == 1
                and selected[0]['name'] == self.contract['comparison'],
                'unique installed constructor comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / self.contract['probe']),
                                          '--clock',
                                          str(build.resolve() / self.contract['clock_binary'])],
                'exact installed constructor CTest command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('entity_constructor_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed constructor comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = self.validate_corpus()
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / self.contract['probe']
        clock = build.resolve() / self.contract['clock_binary']
        actual = observe_fixed(binary, inputs, clock)
        require(len(actual) == self.contract['count'] and same(actual, expected),
                'fresh complete constructor results')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary), clock_sha256=digest(clock))


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
        receipt = dict(complete=True, whole_goal_complete=False,
                       fresh_original_observations=validator.contract['count'],
                       native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Verified original constructor source: 512 Rust outcomes'
          + (' / 512 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
