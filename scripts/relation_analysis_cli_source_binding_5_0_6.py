#!/usr/bin/env python3
"""Bind generated relation analysis to exact source and approved FK expectations."""

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
REGISTRY = ROOT / 'tests/parity/relation_analysis_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same
from compatibility_exceptions_5_0_6 import approved_exception
from compare_generated_relation_analysis import cpp_observation


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
                'locked Rust runtime')
        for checkout, commit in [('teaql-rs', contract['runtime_commit']),
                                 ('teaql_code_gen', contract['generator_commit'])]:
            actual = subprocess.check_output(
                ['git', '-C', str(ROOT / '.reference' / checkout), 'rev-parse', 'HEAD'],
                text=True).strip()
            require(actual == commit, 'locked reference checkout: ' + checkout)
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current source support: ' + path)
        for key in ('corpus', 'exception_manifest', 'source_failure'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        exception = approved_exception('quoted_fk')
        require(exception['exact_relationship_cases'] == contract['exact_count']
                and exception['exception_relationship_cases'] == contract['exception_count']
                and exception['relationship_corpus'] == contract['corpus'],
                'approved bounded relationship exception')
        corpus = read(ROOT / contract['corpus'])
        require(corpus['baseline'] == contract['runtime_commit'],
                'original relationship baseline')
        producer = read(ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json')
        require(same(corpus['producer'], producer)
                and producer['generator_commit'] == contract['generator_commit'],
                'unchanged generated relationship model')
        generated = ROOT / 'tests/generator/parity/dynamic_json_rust'
        require(digest(ROOT / producer['model']) == producer['model_sha256'],
                'unchanged relationship semantic model')
        for name, wanted in producer['files'].items():
            require(digest(generated / name) == wanted,
                    'unchanged generated Rust relationship file: ' + name)
        for name, wanted in corpus['fixtures'].items():
            require(digest(ROOT / 'tests/support' / name) == wanted,
                    'unchanged relationship SQLite fixture: ' + name)

    def validate_source(self, cases_override=None):
        contract = self.registry
        corpus = read(ROOT / contract['corpus'])
        cases = cases_override if cases_override is not None else corpus['cases']
        require(len(cases) == contract['total_count']
                and same(cases, corpus['cases']),
                'complete immutable original/approved case matrix')
        exception = approved_exception('quoted_fk')
        exceptional = [index for index, case in enumerate(cases)
                       if case['qualification'] == 'pending_source_failure_decision']
        require(exceptional == exception['exception_case_indices']
                and len(exceptional) == contract['exception_count'],
                'only approved quoted-FK indices')
        require(all(case['qualification'] in ('matched',
                    'pending_source_failure_decision') for case in cases),
                'only source matches and approved FK exceptions')
        require(len(cases) - len(exceptional) == contract['exact_count'],
                'complete exact source comparison selection')
        for index in exceptional:
            case = cases[index]
            require('cpp_observation_at_review' in case
                    and not same(case['expected'], case['cpp_observation_at_review']),
                    'separate source and approved C++ result: ' + str(index))
        first = cases[exceptional[0]]
        witness = read(ROOT / contract['source_failure'])
        require(same(witness['input'], first['input'])
                and same(witness['original_observation'], first['expected'])
                and same(witness['cpp_observation_at_review'],
                         first['cpp_observation_at_review']),
                'independent quoted-FK failure witness')

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [test['name'] for test in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        scripts = {str(ROOT / contract['comparator'])}
        require({test['name'] for test in tests
                 if len(test.get('command', [])) > 1 and test['command'][1] in scripts}
                == {contract['test']}, 'complete actual relation comparison selection')
        test = next(test for test in tests if test['name'] == contract['test'])
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [str(ROOT / contract['comparator']), '--cpp',
                                           str(build.resolve() / contract['binary'])],
                'exact current relation CTest command')
        properties = {prop['name']: prop['value']
                      for prop in test.get('properties', [])}
        require('relation_analysis_cli' in properties.get('FIXTURES_REQUIRED', []),
                'source fixture on relation comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        contract = self.registry
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        os.environ.pop('TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS', None)
        cases = read(ROOT / contract['corpus'])['cases']
        binary = build.resolve() / contract['binary']
        actual = observe(str(binary), [case['input'] for case in cases])
        exact = 0
        approved = 0
        for case, value in zip(cases, actual):
            if case['qualification'] == 'matched':
                expected = cpp_observation(case['expected'])
                exact += 1
            else:
                expected = case['cpp_observation_at_review']
                approved += 1
            require(same(value, expected),
                    'fresh complete native relationship result: '
                    + str(exact + approved - 1))
        require(exact == contract['exact_count']
                and approved == contract['exception_count'],
                'complete exact/approved native partitions')
        return dict(observations=len(actual), source_comparisons=exact,
                    approved_fk_results=approved, binary_sha256=digest(binary))


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
                      original_observations=7122, exact_source_comparisons=6576,
                      approved_fk_exceptions=546, native=native,
                      registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified relation source matrix: 6576 matches + 546 approved FK exceptions'
          + (' with fresh native results' if native else ''))


if __name__ == '__main__':
    main()
