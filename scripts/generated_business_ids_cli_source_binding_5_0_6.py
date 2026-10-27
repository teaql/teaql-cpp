#!/usr/bin/env python3
"""Bind generated Business-ID CTest to complete original Checker outcomes."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'tests/parity/generated_business_ids_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_values import observe, same
from generate_business_ids import RESOURCE_MESSAGES, adapt_source, cases
from verify_checker_helpers_source_closure_5_0_6 import original_methods


def read(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, label):
    if not value:
        raise RuntimeError(label)


class Validator:
    def __init__(self, registry):
        self.registry = registry

    def validate_files(self):
        contract = self.registry
        require(read(ROOT / 'upstream.lock.json')['commit'] == contract['runtime_commit'],
                'locked runtime')
        for checkout, commit in [('teaql-rs', contract['runtime_commit']),
                                 ('teaql_code_gen', contract['generator_commit'])]:
            actual = subprocess.check_output(
                ['git', '-C', str(ROOT / '.reference' / checkout), 'rev-parse', 'HEAD'],
                text=True).strip()
            require(actual == commit, 'locked checkout: ' + checkout)
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current source: ' + path)
        for key in ('corpus', 'source_closure'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        corpus = read(ROOT / contract['corpus'])
        require(corpus['baseline'] == contract['runtime_commit']
                and corpus['version'] == '5.0.6', 'original corpus baseline')
        producer = read(ROOT / 'tests/generator/parity/business_ids_rust/PROVENANCE.json')
        require(producer == corpus['producer']
                and producer['runtime_commit'] == contract['runtime_commit']
                and producer['generator_commit'] == contract['generator_commit']
                and len(producer['files']) == contract['generated_files'],
                'complete generated SDK identity')
        require(digest(ROOT / producer['model']) == producer['model_sha256'],
                'generated SDK model')
        sdk = ROOT / 'tests/generator/parity/business_ids_rust'
        for path, wanted in producer['files'].items():
            require(digest(sdk / path) == wanted, 'generated SDK file: ' + path)
        closure = read(ROOT / contract['source_closure'])
        require(closure['runtime_commit'] == contract['runtime_commit']
                and closure['observations'] == 1318
                and closure['original_observer_bodies_changed'] is False
                and closure['original_generated_files_changed'] is False
                and closure['reference_registry_versions_preserved'] is True
                and closure['reference_registry_checksums_preserved'] is True,
                'retained original helper source closure')
        for path, wanted in closure['hashes'].items():
            require(digest(ROOT / path) == wanted,
                    'current helper source closure: ' + path)
        source = (ROOT / 'tests/parity/rust_field_layout/src/main.rs').read_text()
        body = original_methods(source, 'fn business_id_probe(',
                                'fn generated_relations_probe(')
        matrix = next(row for row in closure['matrices']
                      if row['project'] == 'rust_business_id_helper_closure_5_0_6')
        require(matrix['observations'] == contract['count']
                and matrix['build_exit_code'] == matrix['comparison_exit_code'] == 0
                and hashlib.sha256(body).hexdigest()
                == matrix['compiled_observer_sha256'],
                'compiled complete original Business-ID observer body')

    def validate_source(self, override=None):
        contract = self.registry
        corpus = override if override is not None else read(ROOT / contract['corpus'])
        rows = corpus['cases']
        source_inputs = [row['input'] for row in rows]
        source_outputs = [row['source_expected'] for row in rows]
        expected = [row['expected'] for row in rows]
        require(len(rows) == contract['count'] and same(source_inputs, cases()),
                'complete original Business-ID input matrix')
        require(corpus['resource_message_mapping'] == RESOURCE_MESSAGES
                and all(same(adapt_source(a), b)
                        for a, b in zip(source_outputs, expected)),
                'complete declared resource type-name adaptation')
        raw = sum(same(a, b) for a, b in zip(source_outputs, expected))
        require(raw == contract['raw_equalities'],
                'original raw Business-ID equality count')
        source_message = next(iter(RESOURCE_MESSAGES))
        messages = sum(1 for row in source_outputs
                       for profile in row['diagnostics']
                       for result in profile
                       if result.get('message') == source_message)
        require(messages == contract['adapted_diagnostics'],
                'declared resource message frequency')

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [row['name'] for row in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        script = str(ROOT / contract['comparator'])
        require({row['name'] for row in tests
                 if len(row.get('command', [])) > 1 and row['command'][1] == script}
                == {contract['test']}, 'complete actual comparison selection')
        test = next(row for row in tests if row['name'] == contract['test'])
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [script, '--cpp',
                                           str(build.resolve() / contract['binary'])],
                'exact current generated Business-ID CTest command')
        properties = {prop['name']: prop['value']
                      for prop in test.get('properties', [])}
        require('generated_business_ids_cli' in properties.get('FIXTURES_REQUIRED', []),
                'source fixture on generated Business-ID comparison')

    def validate_ctest(self, build):
        self.validate_commands(read_ctest(build), build)

    def validate_native(self, build):
        contract = self.registry
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = read(ROOT / contract['corpus'])
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / contract['binary']
        actual = observe(str(binary), inputs)
        require(same(actual, expected),
                'fresh complete generated Business-ID outcomes')
        return dict(observations=len(actual), adapted_equalities=len(actual),
                    raw_source_equalities=contract['raw_equalities'],
                    binary_sha256=digest(binary))


def read_ctest(build):
    return json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
        text=True))['tests']


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, default=REGISTRY)
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
    validator.validate_source()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      original_observations=1044, current_adapted_equalities=1044,
                      raw_source_equalities=916, native=native,
                      registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified generated Business-ID source: 1044 adapted outcomes; '
          '916 raw equalities' + (' with fresh native results' if native else ''))


if __name__ == '__main__':
    main()
