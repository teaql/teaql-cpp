#!/usr/bin/env python3
"""Bind five installed inherited Checker CLIs to one complete original corpus."""

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
REGISTRY = ROOT / 'tests/parity/inherited_checker_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_inherited_checker_helpers_5_0_6 import MODELS, cases
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
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus', 'terminal'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        terminal = read(ROOT / contract['terminal'])
        require(terminal['runtime_commit'] == contract['runtime_commit']
                and terminal['generator_commit'] == contract['generator_commit']
                and terminal['observations'] == contract['count']
                and terminal['models'] == len(MODELS)
                and terminal['normalization'] == 'none'
                and terminal['source_registry_versions_preserved'] is True
                and terminal['source_registry_checksums_preserved'] is True,
                'historical original Checker scope')
        stale = []
        for path, wanted in terminal['hashes'].items():
            if digest(ROOT / path) != wanted:
                stale.append(path)
        require(stale == [contract['historical_comparator_path']],
                'single historical comparator split boundary')
        provenance_path = ROOT / 'tests/generator/parity/inheritance_variants_rust/PROVENANCE.json'
        producer = read(provenance_path)
        require(digest(provenance_path)
                == terminal['original_generated_domain_provenance_sha256']
                and producer['runtime_commit'] == contract['runtime_commit']
                and producer['generator_commit'] == contract['generator_commit']
                and len(producer['files']) == contract['generated_files'],
                'complete original generated inheritance domain')
        require(digest(ROOT / producer['model']) == producer['model_sha256'],
                'original inheritance model')
        sdk = provenance_path.parent
        for path, wanted in producer['files'].items():
            require(digest(sdk / path) == wanted, 'original generated file: ' + path)
        reference = ROOT / '.reference/teaql-rs/Cargo.lock'
        observer = ROOT / 'tests/parity/rust_inherited_checker_helpers_5_0_6/Cargo.lock'
        require(registry_identities(observer) <= registry_identities(reference),
                'reference-seeded observer registry closure')
        wanted_versions = versions(reference)
        local = {'teaql_inherited_checker_helpers_506', 'cpp-inheritance-core',
                 'cpp-inheritance-graph-core', 'cpp-inheritance-variants-core'}
        for package, values in versions(observer).items():
            if package not in local:
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded observer version: ' + package)

    def validate_source(self, override=None):
        contract = self.registry
        retained = read(ROOT / contract['corpus'])
        corpus = override if override is not None else retained
        require(corpus['baseline'] == contract['runtime_commit']
                and len(corpus['cases']) == contract['count']
                and same(corpus['cases'], retained['cases']),
                'complete retained original Checker outcomes')
        inputs = [row['input'] for row in corpus['cases']]
        outputs = [row['expected'] for row in corpus['cases']]
        require(same(inputs, list(cases())) and tuple(MODELS) == tuple(contract['models']),
                'complete five-model original input matrix')
        require(all(sum(row['input']['model'] == model for row in corpus['cases'])
                    == contract['per_model'] for model in MODELS),
                'full original cases per inherited model')
        statistics = dict(callbacks=sum(len(row['hooks']) for row in outputs),
                          policy_reviews=sum(row['plan'] is not None for row in outputs),
                          errors=sum(row['error'] is not None for row in outputs),
                          findings=sum(len(row['diagnostics'][0]) for row in outputs),
                          evidence=sum(len(row['evidence']) for row in outputs))
        require(statistics == contract['statistics']
                and statistics == read(ROOT / contract['terminal'])['statistics'],
                'complete Checker, Policy and fix-evidence boundaries')
        require(all(row['wall_millis'] == source['wall_millis']
                    for source, row in zip(inputs, outputs)),
                'controlled original wall-clock outcomes')

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [row['name'] for row in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        script = str(ROOT / contract['comparator'])
        selected = {row['name'] for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script}
        expected_names = {'inherited_checker_helpers_' + model + '_rust_parity'
                          for model in MODELS}
        require(selected == expected_names, 'complete five-model CTest selection')
        for model in MODELS:
            test = next(row for row in tests
                        if row['name'] == 'inherited_checker_helpers_' + model + '_rust_parity')
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [script, '--cpp-dir', str(build.resolve()),
                                               '--model', model, '--clock',
                                               str(build.resolve() / contract['clock'])],
                    'exact current ' + model + ' CTest command')
            properties = {prop['name']: prop['value']
                          for prop in test.get('properties', [])}
            require('inherited_checker_cli' in properties.get('FIXTURES_REQUIRED', []),
                    'source fixture on ' + model + ' comparison')

    def validate_ctest(self, build):
        self.validate_commands(read_ctest(build), build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        contract = self.registry
        corpus = read(ROOT / contract['corpus'])
        all_inputs = [row['input'] for row in corpus['cases']]
        all_expected = [row['expected'] for row in corpus['cases']]
        clock = build.resolve() / contract['clock']
        observations = {}
        for model in MODELS:
            indexes = [index for index, value in enumerate(all_inputs)
                       if value['model'] == model]
            inputs = [all_inputs[index] for index in indexes]
            expected = [all_expected[index] for index in indexes]
            binary = build.resolve() / ('teaql_inherited_checker_helpers_' + model + '_probe')
            actual = observe_fixed(binary, inputs, clock)
            require(same(actual, expected), 'fresh complete ' + model + ' outcomes')
            observations[model] = dict(observations=len(actual),
                                       exact_equalities=len(actual),
                                       binary_sha256=digest(binary))
        return dict(models=observations, clock_sha256=digest(clock),
                    observations=sum(row['observations'] for row in observations.values()),
                    exact_equalities=sum(row['exact_equalities'] for row in observations.values()))


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
                      original_observations=7200,
                      current_exact_equalities=native['exact_equalities'] if native else None,
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified inherited Checker source: 5 models / 7200 outcomes'
          + (' with fresh exact native results' if native else ''))


if __name__ == '__main__':
    main()
