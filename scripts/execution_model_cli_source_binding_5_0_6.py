#!/usr/bin/env python3
"""Bind accepted execution-result and hot-cache source streams to native CLIs."""

import argparse
import gzip
import hashlib
import importlib
import json
import os
from pathlib import Path
import resource
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'tests/parity/execution_model_cli_contracts_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
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
        registry = self.registry
        require(read(ROOT / 'upstream.lock.json')['commit'] == registry['runtime_commit'],
                'locked Rust runtime')
        for checkout, commit in [('teaql-rs', registry['runtime_commit']),
                                 ('teaql_code_gen', registry['generator_commit'])]:
            actual = subprocess.check_output(
                ['git', '-C', str(ROOT / '.reference' / checkout), 'rev-parse', 'HEAD'],
                text=True).strip()
            require(actual == commit, 'locked reference checkout: ' + checkout)
        require([item['name'] for item in registry['contracts']]
                == ['execution_result_models', 'execution_result_model_states',
                    'hot_execution'], 'complete selected execution source commands')
        for path, wanted in registry['support_files'].items():
            require(digest(ROOT / path) == wanted, 'unchanged source support: ' + path)
        for item in registry['contracts']:
            name = item['name']
            for key in ('corpus', 'paired', 'terminal'):
                path = item[key]
                require(digest(ROOT / path) == item[key + '_sha256'],
                        'retained ' + key + ': ' + name)
            terminal = read(ROOT / item['terminal'])
            require(terminal['runtime_commit'] == registry['runtime_commit']
                    and terminal['generator_commit'] == registry['generator_commit'],
                    'accepted terminal baseline: ' + name)
            for key in ('corpus', 'paired', 'source_raw'):
                if key in item:
                    path = item[key]
                    require(terminal['artifacts'][path] == digest(ROOT / path),
                            'terminal receipt names retained ' + key + ': ' + name)
            if 'source_raw' in item:
                require(digest(ROOT / item['source_raw']) == item['source_raw_sha256'],
                        'retained raw original stream: ' + name)
            module = importlib.import_module(Path(item['comparator']).stem)
            corpus = read(ROOT / item['corpus'])
            identity = module.identity()
            for key, value in identity.items():
                require(same(corpus[key], value), 'current original identity: '
                        + name + '/' + key)

    def validate_source(self, overrides=None):
        for item in self.registry['contracts']:
            name = item['name']
            module = importlib.import_module(Path(item['comparator']).stem)
            corpus = read(ROOT / item['corpus'])
            paired = (overrides or {}).get(name)
            if paired is None:
                paired = read(ROOT / item['paired'])
            inputs = [row['input'] for row in corpus['cases']]
            expected = [row['expected'] for row in corpus['cases']]
            require(len(inputs) == item['count']
                    and same(inputs, module.inputs())
                    and same(inputs, paired['inputs']),
                    'complete source input selection: ' + name)
            require(len(paired['original']) == item['count']
                    and same(paired['original'], expected),
                    'complete raw original results: ' + name)
            require(len(paired['cpp']) == item['count']
                    and same(paired['cpp'], expected)
                    and paired['failures'] == [],
                    'accepted complete paired result: ' + name)
            module.check_source(inputs, expected)
            if name == 'hot_execution':
                raw = read(ROOT / item['source_raw'])
                require(all(same(raw[key], corpus[key]) for key in
                            ('baseline', 'producer', 'sources'))
                        and same(raw['inputs'], inputs)
                        and same(raw['original'], expected)
                        and sum(len(row) for row in expected) == 6480,
                        'independent complete original hot-cache stream')

    def validate_commands(self, tests, build):
        names = [test['name'] for test in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        by_name = {test['name']: test for test in tests}
        scripts = {str(ROOT / item['comparator']) for item in self.registry['contracts']}
        require({test['name'] for test in tests
                 if len(test.get('command', [])) > 1 and test['command'][1] in scripts}
                == {item['test'] for item in self.registry['contracts']},
                'complete actual execution comparison selection')
        for item in self.registry['contracts']:
            test = by_name[item['test']]
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [str(ROOT / item['comparator']), '--cpp',
                                               str(build.resolve() / item['binary'])],
                    'exact current CTest command: ' + item['name'])
            properties = {prop['name']: prop['value']
                          for prop in test.get('properties', [])}
            require('execution_model_cli' in properties.get('FIXTURES_REQUIRED', []),
                    'source fixture on comparison: ' + item['name'])

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        rows = []
        for item in self.registry['contracts']:
            corpus = read(ROOT / item['corpus'])
            inputs = [row['input'] for row in corpus['cases']]
            expected = [row['expected'] for row in corpus['cases']]
            binary = build.resolve() / item['binary']
            actual = observe(str(binary), inputs)
            require(same(actual, expected), 'fresh complete native results: ' + item['name'])
            if item['name'] == 'hot_execution':
                require(sum(len(row) for row in actual) == 6480,
                        'fresh native hot-cache execution rounds')
            rows.append(dict(name=item['name'], observations=len(actual),
                             execution_rounds=(6480 if item['name'] == 'hot_execution'
                                               else len(actual)),
                             binary_sha256=digest(binary)))
        return rows


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
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else []
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      source_scenarios=4448, hot_execution_rounds=6480,
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified execution model sources: 3 complete matrices / 4448 scenarios'
          + (' with 6480 fresh hot-cache rounds' if native else ''))


if __name__ == '__main__':
    main()
