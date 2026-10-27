#!/usr/bin/env python3
"""Bind the installed typed Business-ID lookup CLI to retained original Q outcomes."""

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
REGISTRY = ROOT / 'tests/parity/business_id_lookup_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_business_id_lookups_5_0_6 import identity, inputs
from compare_values import observe, same


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
        for key in ('corpus', 'runtime_pair', 'generated_pair', 'terminal'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        corpus = read(ROOT / contract['corpus'])
        for key, value in identity().items():
            require(same(corpus[key], value), 'original Q identity: ' + key)
        terminal = read(ROOT / contract['terminal'])
        require(terminal['runtime_commit'] == contract['runtime_commit']
                and terminal['generator_commit'] == contract['generator_commit']
                and terminal['counts']['paired'] == contract['count'],
                'terminal baseline and count')
        for key in ('corpus', 'runtime_pair', 'generated_pair'):
            path = contract[key]
            require(terminal['artifacts'][path] == digest(ROOT / path),
                    'terminal artifact: ' + key)

    def validate_source(self, override=None):
        contract = self.registry
        corpus = read(ROOT / contract['corpus'])
        generated = override if override is not None else read(ROOT / contract['generated_pair'])
        preliminary = read(ROOT / contract['runtime_pair'])
        cases = corpus['cases']
        source_inputs = [row['input'] for row in cases]
        expected = [row['expected'] for row in cases]
        require(len(cases) == contract['count'] and same(source_inputs, inputs()),
                'complete original Q input matrix')
        for label, pair, composition in [('runtime', preliminary, True),
                                         ('generated', generated, False)]:
            require(pair['runtime_composition'] is composition
                    and pair['failures'] == []
                    and len(pair['inputs']) == len(pair['original'])
                    == len(pair['cpp']) == contract['count']
                    and same(pair['inputs'], source_inputs)
                    and same(pair['original'], expected)
                    and same(pair['cpp'], expected),
                    'complete retained ' + label + ' Q outcomes')
        for row in expected:
            if 'lookup' in row:
                require(same(row['lookup'], row['manual']),
                        'original lookup/manual composition')

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [test['name'] for test in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        script = str(ROOT / contract['comparator'])
        require({row['name'] for row in tests
                 if len(row.get('command', [])) > 1 and row['command'][1] == script}
                == {contract['test']}, 'complete actual comparison selection')
        test = next(row for row in tests if row['name'] == contract['test'])
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [script, '--cpp',
                                           str(build.resolve() / contract['binary'])],
                'exact current Q CTest command')
        properties = {prop['name']: prop['value']
                      for prop in test.get('properties', [])}
        require('business_id_lookup_cli' in properties.get('FIXTURES_REQUIRED', []),
                'source fixture on Q comparison')

    def validate_ctest(self, build):
        tests = read_ctest(build)
        self.validate_commands(tests, build)

    def validate_native(self, build):
        contract = self.registry
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        os.environ['TEAQL_BUSINESS_ID_KEY_VERSION'] = '1'
        os.environ['TEAQL_BUSINESS_ID_KEY_V1_HEX'] = bytes(range(32)).hex()
        corpus = read(ROOT / contract['corpus'])
        cases = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / contract['binary']
        actual = observe(str(binary), cases)
        require(same(actual, expected), 'fresh complete generated-factory Q outcomes')
        require(sum('logs' in row and row['logs'] != [] for row in actual)
                == contract['safe_sql_cases'], 'safe SQL record count')
        return dict(observations=len(actual), raw_equalities=len(actual),
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
                      original_observations=528, current_raw_equalities=528,
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(), mtime=0, compresslevel=3))
    print('Verified typed Business-ID Q source: 528 exact outcomes'
          + (' with fresh native results' if native else ''))


if __name__ == '__main__':
    main()
