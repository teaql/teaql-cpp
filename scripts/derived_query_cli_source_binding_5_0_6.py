#!/usr/bin/env python3
"""Bind derived-query composition and diagnostics CLIs to original results."""

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
REGISTRY = ROOT / 'tests/parity/derived_query_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same
from generate_derived_query_composition_source_5_0_6 import inputs as composition_inputs
from generate_derived_query_diagnostics_5_0_6 import inputs as diagnostics_inputs, metadata


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
        for key in ('composition_corpus', 'diagnostics_corpus',
                    'composition_source_receipt', 'composition_final_receipt',
                    'diagnostics_final_receipt', 'diagnostics_source_review'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        source = metadata()
        require(source['baseline'] == contract['runtime_commit']
                and source['adaptation'].startswith('None.'),
                'original derived-query metadata')
        old_composition = read(ROOT / contract['composition_source_receipt'])
        old_diagnostics = read(ROOT / contract['diagnostics_final_receipt'])
        final_composition = read(ROOT / contract['composition_final_receipt'])
        source_review = read(ROOT / contract['diagnostics_source_review'])
        require(old_composition['baseline'] == old_diagnostics['baseline']
                == final_composition['baseline'] == contract['runtime_commit'],
                'historical receipt baselines')
        require(old_composition['corpus_sha256']
                == digest(ROOT / contract['composition_corpus'])
                and old_diagnostics['corpus_sha256']
                == digest(ROOT / contract['diagnostics_corpus']),
                'retained original corpus identities')
        require(same(old_composition['source_hashes'], source['source_hashes'])
                and source_review['source_hashes'].items()
                >= source['source_hashes'].items(),
                'same locked original diagnostic sources')
        require(old_composition['cases'] == 360
                and final_composition['reproduction_compare']['observations'] == 360
                and final_composition['reproduction_compare']['exit_code'] == 0
                and old_diagnostics['observations']['cases'] == 576
                and old_diagnostics['observations']['normalization'] == 'none',
                'historical composition and diagnostic scopes')
        require(final_composition['source_sha256']
                == digest(ROOT / final_composition['source']),
                'current composition probe preserved')
        stale_production = [path for path, wanted in
                            old_diagnostics['production_sha256'].items()
                            if digest(ROOT / path) != wanted]
        require(len(stale_production) == self.registry['historical_production_hashes_stale'],
                'historical diagnostic production hashes remain separate')

    def validate_source(self, overrides=None):
        contract = self.registry
        overrides = overrides or {}
        source = metadata()
        for name, expected_inputs in [('composition', list(composition_inputs())),
                                      ('diagnostics', list(diagnostics_inputs()))]:
            cohort = contract['cohorts'][name]
            corpus = overrides.get(name)
            if corpus is None:
                corpus = read(ROOT / contract[name + '_corpus'])
            retained = read(ROOT / contract[name + '_corpus'])
            require(len(corpus['cases']) == cohort['count']
                    and same(corpus['cases'], retained['cases'])
                    and all(same(corpus[key], value) for key, value in source.items()),
                    'complete ' + name + ' source metadata and count')
            cases = [row['input'] for row in corpus['cases']]
            results = [row['expected'] for row in corpus['cases']]
            require(same(cases, expected_inputs),
                    'complete ' + name + ' original input matrix')
            require(sum(row['error'] is None for row in results) == cohort['successes']
                    and sum(row['error'] is not None for row in results) == cohort['errors']
                    and sum(len(row['logs']) for row in results) == cohort['sql_logs'],
                    'complete ' + name + ' source outcome boundaries')
            self.validate_privacy(cases, results, name)

    @staticmethod
    def validate_privacy(cases, results, name):
        for case, output in zip(cases, results):
            if not case['logging'] or case['invalid_intent']:
                require(output['logs'] == [], name + ' disabled/invalid trace')
            rendered = json.dumps(output['logs'], ensure_ascii=False)
            secrets = ([case['secret'], 'PRIVATE-FUTURE-A', 'PRIVATE-FUTURE-B']
                       if name == 'composition' else [case['secret']])
            for secret in secrets:
                require(secret not in rendered, name + ' sensitive SQL log')
            if case['logging'] and not case['failure']:
                require(output['independent_logs'][0]['comment'] == case['secret'],
                        name + ' independent query intent')

    def validate_commands(self, tests, build):
        names = [row['name'] for row in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        for name, cohort in self.registry['cohorts'].items():
            script = str(ROOT / cohort['comparator'])
            require({row['name'] for row in tests
                     if len(row.get('command', [])) > 1 and row['command'][1] == script}
                    == {cohort['test']}, 'complete ' + name + ' comparison selection')
            test = next(row for row in tests if row['name'] == cohort['test'])
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [script, '--cpp',
                                               str(build.resolve() / cohort['binary'])],
                    'exact current ' + name + ' CTest command')
            properties = {prop['name']: prop['value']
                          for prop in test.get('properties', [])}
            require('derived_query_cli' in properties.get('FIXTURES_REQUIRED', []),
                    'source fixture on ' + name + ' comparison')
        self_test = next(row for row in tests
                         if row['name'] == 'derived_query_diagnostics_context_contracts')
        require(self_test['command'] == [
                    str(build.resolve() / self.registry['cohorts']['diagnostics']['binary']),
                    '--self-test'], 'derived diagnostic Context self-test command')

    def validate_ctest(self, build):
        self.validate_commands(read_ctest(build), build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        observations = {}
        for name, cohort in self.registry['cohorts'].items():
            corpus = read(ROOT / self.registry[name + '_corpus'])
            cases = [row['input'] for row in corpus['cases']]
            expected = [row['expected'] for row in corpus['cases']]
            binary = build.resolve() / cohort['binary']
            actual = observe(str(binary), cases)
            require(same(actual, expected), 'fresh complete ' + name + ' outcomes')
            self.validate_privacy(cases, actual, name)
            observations[name] = dict(observations=len(actual),
                                      exact_equalities=len(actual),
                                      binary_sha256=digest(binary))
        return observations


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
                      original_observations=936,
                      current_exact_equalities=936 if native else None,
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified derived-query source: 360 composition + 576 diagnostics'
          + (' with fresh exact native results' if native else ''))


if __name__ == '__main__':
    main()
