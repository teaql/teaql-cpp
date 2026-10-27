#!/usr/bin/env python3
"""Bind complete original stream shapes to the installed strict native CLI."""

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
REGISTRY = ROOT / 'tests/parity/stream_shapes_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same
from compare_stream_shapes_5_0_6 import identity, inputs, verify_boundaries


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
        for key in ('corpus', 'historical_pair', 'strict_receipt', 'terminal'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        corpus = read(ROOT / contract['corpus'])
        for key, value in identity().items():
            require(same(corpus[key], value),
                    'current original stream identity: ' + key)
        terminal = read(ROOT / contract['terminal'])
        require(terminal['runtime_commit'] == contract['runtime_commit']
                and terminal['generator_commit'] == contract['generator_commit']
                and terminal['original_observations'] == contract['count'],
                'accepted stream terminal scope')
        for key in ('corpus', 'historical_pair'):
            path = contract[key]
            require(terminal['artifacts'][path] == digest(ROOT / path),
                    'terminal artifact identity: ' + key)
        strict = read(ROOT / contract['strict_receipt'])
        require(strict['corpus_sha256'] == digest(ROOT / contract['corpus'])
                and strict['complete'] is True
                and strict['source_promoted'] is False
                and strict['cases'] == contract['count']
                and strict['inputs_unchanged'] is True,
                'retained strict original/current readback')

    def validate_source(self, override=None):
        contract = self.registry
        corpus = read(ROOT / contract['corpus'])
        strict = override if override is not None else read(ROOT / contract['strict_receipt'])
        old = read(ROOT / contract['historical_pair'])
        cases = corpus['cases']
        source_inputs = [row['input'] for row in cases]
        expected = [row['expected'] for row in cases]
        require(len(cases) == contract['count']
                and same(source_inputs, inputs())
                and same(source_inputs, old['inputs']),
                'complete original stream input matrix')
        require(len(old['original']) == contract['count']
                and same(old['original'], expected),
                'retained complete original output stream')
        require(len(old['raw_failures']) == contract['historical_adaptations']
                and len(old['startup_adaptations']) == contract['historical_adaptations']
                and old['failures'] == [],
                'historical adapted comparison remains separate')
        require(strict['strict_source_differences'] == []
                and strict['obsolete_startup_expectation_differences']
                == old['raw_failures']
                and strict['boundaries_passed'] is True
                and len(strict['actual']) == contract['count']
                and same(strict['actual'], expected),
                'complete retained raw-equal native stream')
        verify_boundaries(source_inputs, expected, strict['actual'])

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [test['name'] for test in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        scripts = {str(ROOT / contract['comparator'])}
        require({test['name'] for test in tests
                 if len(test.get('command', [])) > 1 and test['command'][1] in scripts}
                == {contract['test']}, 'complete actual stream comparison selection')
        test = next(test for test in tests if test['name'] == contract['test'])
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [str(ROOT / contract['comparator']), '--cpp',
                                           str(build.resolve() / contract['binary'])],
                'exact current stream CTest command')
        properties = {prop['name']: prop['value']
                      for prop in test.get('properties', [])}
        require('stream_shapes_cli' in properties.get('FIXTURES_REQUIRED', []),
                'source fixture on stream comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        contract = self.registry
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = read(ROOT / contract['corpus'])
        source_inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / contract['binary']
        actual = observe(str(binary), source_inputs)
        require(same(actual, expected), 'fresh complete strict native stream')
        verify_boundaries(source_inputs, expected, actual)
        return dict(observations=len(actual), raw_equalities=len(actual),
                    startup_adaptations=0, binary_sha256=digest(binary))


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
                      original_observations=13104, historical_adaptations=6012,
                      current_raw_equalities=13104, native=native,
                      registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified stream source: 13104 strict results; historical 6012 adaptations'
          + (' with fresh native outputs' if native else ''))


if __name__ == '__main__':
    main()
