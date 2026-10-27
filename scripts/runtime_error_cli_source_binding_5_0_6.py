#!/usr/bin/env python3
"""Bind retained complete Rust error execution to current native CTest consumers."""

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
REGISTRY = ROOT / 'tests/parity/runtime_error_cli_source_contracts_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, observe, same


def read(path):
    data = Path(path).read_bytes()
    return json.loads(gzip.decompress(data) if Path(path).suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, label):
    if not condition:
        raise RuntimeError(label)


class Validator:
    def __init__(self, registry):
        self.registry = registry
        self.execution = read(ROOT / registry['original_execution']['path'])
        self.acceptance = read(ROOT / registry['source_acceptance']['path'])

    def validate_files(self):
        r = self.registry
        require(digest(ROOT / r['original_execution']['path']) == r['original_execution']['sha256'],
                'retained original execution identity')
        require(digest(ROOT / r['source_acceptance']['path']) == r['source_acceptance']['sha256'],
                'retained source acceptance identity')
        require(read(ROOT / 'upstream.lock.json')['commit'] == r['runtime_commit'],
                'current locked runtime')
        for name, commit in [('teaql-rs', r['runtime_commit']),
                             ('teaql_code_gen', r['generator_commit'])]:
            actual = subprocess.check_output(['git', '-C', str(ROOT / '.reference' / name),
                                              'rev-parse', 'HEAD'], text=True).strip()
            require(actual == commit, 'unchanged reference checkout: ' + name)
        for path, wanted in r['support_files'].items():
            require(digest(ROOT / path) == wanted, 'source binding support: ' + path)
        for path, wanted in r['original_files'].items():
            require(digest(ROOT / path) == wanted, 'unchanged original/source body: ' + path)
        admission = read(ROOT / r['admission']['path'])
        require(digest(ROOT / r['admission']['path']) == r['admission']['sha256']
                and admission['complete'] is True
                and admission['reproduced_false_acceptances'] == 7
                and admission['installed_rejections'] == 7,
                'actual comparator admission and preceding byte recovery')
        for comparator in admission['preceding_comparators'].values():
            require(digest(ROOT / comparator['path']) == comparator['current_sha256'],
                    'installed comparator identity')

    def validate_source(self, execution=None):
        r = self.registry
        source = execution if execution is not None else self.execution
        require(source['complete'] is True and source['full_goal_accepted'] is False,
                'bounded durable source completion')
        require(source['baseline'] == r['runtime_commit']
                and source['original_trait_tests'] == 3
                and source['original_compile_refusals'] == 4,
                'original Rust source/trait selection')
        require(len(source['steps']) == 24
                and all(type(step['exit_code']) is int and step['exit_code'] == 0
                        for step in source['steps']), 'complete original/current execution steps')
        require(source['steps'][1]['command'][:4] == ['cargo', 'build', '--offline', '--locked']
                and source['steps'][2]['command'][:4] == ['cargo', 'test', '--offline', '--locked']
                and source['steps'][3]['command'][:4] == ['cargo', 'test', '--offline', '--locked'],
                'locked original build and trait tests')
        require(source['profiles']['native']['passed'] == 7
                and source['profiles']['modules_native']['passed'] == 6,
                'historical native/imported selected test scopes')
        require(self.acceptance['complete'] is True
                and self.acceptance['full_goal_accepted'] is False
                and self.acceptance['runtime_commit'] == r['runtime_commit']
                and self.acceptance['receipts'][r['original_execution']['path']]
                == r['original_execution']['sha256'],
                'accepted original execution receipt')
        require(len(source['matrices']) == len(r['contracts']) == 3,
                'complete three original error matrices')
        for contract, matrix, step_index in zip(r['contracts'], source['matrices'], (4, 5, 6)):
            corpus = read(ROOT / contract['corpus'])
            require(digest(ROOT / contract['corpus']) == contract['corpus_sha256'],
                    'immutable original error corpus')
            require(corpus['baseline'] == r['runtime_commit']
                    and len(corpus['cases']) == contract['count']
                    and all(type(row) is dict and set(row) == {'input', 'expected'}
                            for row in corpus['cases']), 'complete exact case rows')
            require(matrix['name'] == Path(contract['corpus']).name
                    and matrix['observations'] == contract['count'],
                    'historical matrix identity and count')
            expected = [row['expected'] for row in corpus['cases']]
            require(same(matrix['original_outputs'], expected),
                    'original Rust outputs equal exact retained corpus')
            require(same(json_lines(source['steps'][step_index]['stdout']), expected),
                    'complete raw original Rust stream')
            require(source['steps'][step_index]['stderr'] == '',
                    'original observer stderr')
        require(sum(contract['count'] for contract in r['contracts']) == 2362,
                'complete overlapping error observation count')

    def validate_commands(self, tests, build):
        by_name = {test['name']: test for test in tests}
        require(len(by_name) == len(tests), 'unique CTest names')
        scripts = {str(ROOT / contract['comparator']) for contract in self.registry['contracts']}
        require({test['name'] for test in tests if len(test.get('command', [])) > 1
                 and test['command'][1] in scripts}
                == {contract['test'] for contract in self.registry['contracts']},
                'complete actual error comparison entry selection')
        for contract in self.registry['contracts']:
            test = by_name[contract['test']]
            command = test['command']
            require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                    and command[1:] == [str(ROOT / contract['comparator']), '--cpp',
                                       str(build.resolve() / contract['binary']),
                                       '--cpp-emulator='],
                    'exact current error comparison CTest command')
            props = {property['name']: property['value']
                     for property in test.get('properties', [])}
            require('runtime_error_cli_source' in props.get('FIXTURES_REQUIRED', []),
                    'error source prerequisite on actual comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        widths = subprocess.check_output(
            [str(build.resolve() / 'teaql_runtime_check_probe'), '--pointer-bits'], text=True).strip()
        require(widths == '64', 'retained error corpora are native64-only')
        results = []
        for contract in self.registry['contracts']:
            corpus = read(ROOT / contract['corpus'])
            inputs = [row['input'] for row in corpus['cases']]
            expected = [row['expected'] for row in corpus['cases']]
            binary = build.resolve() / contract['binary']
            actual = observe(str(binary), inputs)
            require(same(actual, expected), 'current native error outputs: ' + contract['name'])
            results.append(dict(name=contract['name'], count=len(actual),
                                binary_sha256=digest(binary)))
        return results


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
    require(args.record is None or not args.record.exists(), 'preserve historical evidence')
    validator = Validator(read(args.contracts))
    validator.validate_files()
    validator.validate_source()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else []
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      historical_original_observations=2362,
                      current_native_observations=sum(row['count'] for row in native),
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('Verified retained Rust error source: 3 matrices / 2362 original outcomes'
          + (' and current native outputs' if native else ''))


if __name__ == '__main__':
    main()
