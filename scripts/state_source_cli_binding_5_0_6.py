#!/usr/bin/env python3
"""Bind four accepted state/entity source replays to current native CTest CLIs."""

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
REGISTRY = ROOT / 'tests/parity/state_source_cli_contracts_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same


def read(path):
    data = Path(path).read_bytes()
    return json.loads(gzip.decompress(data) if Path(path).suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, label):
    if not value:
        raise RuntimeError(label)


class Validator:
    def __init__(self, registry):
        self.registry = registry

    def validate_files(self):
        r = self.registry
        require(read(ROOT / 'upstream.lock.json')['commit'] == r['runtime_commit'],
                'locked Rust runtime')
        for name, commit in [('teaql-rs', r['runtime_commit']),
                             ('teaql_code_gen', r['generator_commit'])]:
            actual = subprocess.check_output(['git', '-C', str(ROOT / '.reference' / name),
                                              'rev-parse', 'HEAD'], text=True).strip()
            require(actual == commit, 'unchanged reference checkout: ' + name)
        require([item['name'] for item in r['contracts']] == [
            'runtime_state_bridge', 'generated_runtime_state_bridge',
            'generated_load_state_bulk', 'entity_source'],
            'complete four state/entity comparison selection')
        for path, wanted in r['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current source-binding support: ' + path)
        for contract in r['contracts']:
            require(digest(ROOT / contract['corpus']) == contract['corpus_sha256'],
                    'unchanged complete corpus: ' + contract['name'])
            require(digest(ROOT / contract['source_receipt']['path'])
                    == contract['source_receipt']['sha256'],
                    'retained original execution: ' + contract['name'])
            require(digest(ROOT / contract['acceptance']['path'])
                    == contract['acceptance']['sha256'],
                    'terminal source acceptance: ' + contract['name'])
            corpus = read(ROOT / contract['corpus'])
            require(corpus['runtime_commit'] == r['runtime_commit'],
                    'corpus runtime baseline: ' + contract['name'])
            require(corpus['source_hashes'] == contract['source_hashes'],
                    'complete source selection: ' + contract['name'])
            for path, wanted in contract['source_hashes'].items():
                require(digest(ROOT / path) == wanted,
                        'current original/generated source identity: ' + path)

    def validate_source(self, overrides=None):
        for contract in self.registry['contracts']:
            name = contract['name']
            corpus = read(ROOT / contract['corpus'])
            source = (overrides or {}).get(name)
            if source is None:
                source = read(ROOT / contract['source_receipt']['path'])
            acceptance = read(ROOT / contract['acceptance']['path'])
            require(acceptance['complete'] is True
                    and acceptance['evidence'][contract['source_receipt']['path']]
                    == contract['source_receipt']['sha256'],
                    'accepted immutable original receipt: ' + name)
            require(source['complete'] is True,
                    'complete original source corpus: ' + name)
            if 'historical_corpus' in contract:
                historical = contract['historical_corpus']
                raw = subprocess.check_output([
                    'git', '-C', str(ROOT), 'show',
                    historical['commit'] + ':' + contract['corpus']])
                require(hashlib.sha256(raw).hexdigest() == historical['sha256']
                        and source['corpus_sha256'] == historical['sha256'],
                        'retained original corpus blob: ' + name)
                previous = json.loads(gzip.decompress(raw))
                previous_hashes = dict(previous['source_hashes'])
                require(previous_hashes.pop(historical['changed_source'])
                        == historical['previous_source_sha256'],
                        'historical source metadata: ' + name)
                current_hashes = dict(corpus['source_hashes'])
                require(current_hashes.pop(historical['changed_source'])
                        == historical['current_source_sha256'],
                        'current source metadata: ' + name)
                historical_source = subprocess.check_output([
                    'git', '-C', str(ROOT), 'show',
                    historical['commit'] + ':' + historical['changed_source']])
                require(hashlib.sha256(historical_source).hexdigest()
                        == historical['current_source_sha256'],
                        'historical source body behind stale metadata: ' + name)
                require(previous_hashes == current_hashes
                        and {key: value for key, value in previous.items()
                             if key != 'source_hashes'}
                        == {key: value for key, value in corpus.items()
                            if key != 'source_hashes'},
                        'only approved corpus metadata transition: ' + name)
            else:
                require(source['corpus_sha256'] == contract['corpus_sha256'],
                        'complete original source corpus: ' + name)
            require(len(corpus['inputs']) == contract['count']
                    and len(corpus[contract['source_expected']]) == contract['count']
                    and len(corpus[contract['cpp_expected']]) == contract['count'],
                    'complete original/native matrix: ' + name)
            if name == 'generated_load_state_bulk':
                require(source['exit_code'] == 0
                        and len(json_lines(source['stdout'])) == 12
                        and same(json_lines(source['stdout']), corpus['expected'])
                        and sum(len(value['rows']) for value in corpus['expected']) == 40404,
                        'complete original typed-batch rows')
            else:
                require(source['actual_original_replayed'] is True
                        and source['observations'] == contract['count']
                        and len(source['steps']) == 2,
                        'complete original replay: ' + name)
                by_label = {step['label']: step for step in source['steps']}
                require(set(by_label) == {contract['source_label'], 'cpp'}
                        and all(step['exit_code'] == 0 for step in source['steps']),
                        'successful original/native source steps: ' + name)
                original = by_label[contract['source_label']]['stdout']
                require(same(json_lines(original), corpus[contract['source_expected']]),
                        'complete raw original source stream: ' + name)
                require(same(json_lines(by_label['cpp']['stdout']),
                             corpus[contract['cpp_expected']]),
                        'retained native adaptation and complete output: ' + name)

    def validate_commands(self, tests, build):
        by_name = {test['name']: test for test in tests}
        require(len(by_name) == len(tests), 'unique CTest names')
        scripts = {str(ROOT / item['comparator']) for item in self.registry['contracts']}
        require({test['name'] for test in tests if len(test.get('command', [])) > 1
                 and test['command'][1] in scripts}
                == {item['test'] for item in self.registry['contracts']},
                'complete actual state/entity CTest selection')
        for contract in self.registry['contracts']:
            test = by_name[contract['test']]
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [str(ROOT / contract['comparator']), '--cpp',
                                               str(build.resolve() / contract['binary']),
                                               *contract['extra_args']],
                    'exact current CTest command: ' + contract['name'])
            props = {item['name']: item['value'] for item in test.get('properties', [])}
            require('state_source_cli' in props.get('FIXTURES_REQUIRED', []),
                    'source prerequisite on actual CLI: ' + contract['name'])

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        rows = []
        for contract in self.registry['contracts']:
            corpus = read(ROOT / contract['corpus'])
            binary = build.resolve() / contract['binary']
            payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                              for value in corpus['inputs'])
            result = subprocess.run([str(binary)], input=payload, text=True,
                                    capture_output=True, check=True)
            require(result.stderr == '', 'current native stderr: ' + contract['name'])
            actual = json_lines(result.stdout)
            require(len(actual) == contract['count']
                    and same(actual, corpus[contract['cpp_expected']]),
                    'fresh current native output: ' + contract['name'])
            count = (sum(len(value['rows']) for value in actual)
                     if contract['name'] == 'generated_load_state_bulk' else len(actual))
            rows.append(dict(name=contract['name'], observations=len(actual),
                             row_values=count, binary_sha256=digest(binary)))
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
    require(args.record is None or not args.record.exists(), 'preserve historical evidence')
    validator = Validator(read(args.contracts))
    validator.validate_files()
    validator.validate_source()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else []
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      source_observations=272, typed_batch_rows=40404,
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('Verified accepted state/entity sources: 4 complete matrices / 272 observations'
          + (' with fresh native outputs and 40404 typed rows' if native else ''))


if __name__ == '__main__':
    main()
