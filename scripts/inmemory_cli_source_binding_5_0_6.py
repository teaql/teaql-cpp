#!/usr/bin/env python3
"""Bind complete accepted Rust in-memory source evidence to the current CLI."""

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
REGISTRY = ROOT / 'tests/parity/inmemory_cli_source_contracts_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, observe, same


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
        self.source = read(ROOT / registry['source_execution']['path'])
        self.acceptance = read(ROOT / registry['source_acceptance']['path'])

    def validate_files(self):
        r = self.registry
        require(digest(ROOT / r['source_execution']['path']) == r['source_execution']['sha256'],
                'complete retained source execution identity')
        require(digest(ROOT / r['source_acceptance']['path']) == r['source_acceptance']['sha256'],
                'accepted complete source identity')
        require(read(ROOT / 'upstream.lock.json')['commit'] == r['runtime_commit'],
                'locked Rust 5.0.6 baseline')
        for name, commit in [('teaql-rs', r['runtime_commit']),
                             ('teaql_code_gen', r['generator_commit'])]:
            actual = subprocess.check_output(['git', '-C', str(ROOT / '.reference' / name),
                                              'rev-parse', 'HEAD'], text=True).strip()
            require(actual == commit, 'reference checkout: ' + name)
        require(set(r['original_files']) == set(self.source['sources']),
                'complete original/production source file selection')
        for path, entry in self.source['sources'].items():
            require(digest(ROOT / path) == entry['sha256'] == r['original_files'][path]
                    and hashlib.sha256(entry['text'].encode()).hexdigest() == entry['sha256'],
                    'unchanged full source body: ' + path)
        for path, wanted in r['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current CLI/source support: ' + path)
        admission = read(ROOT / r['admission']['path'])
        require(digest(ROOT / r['admission']['path']) == r['admission']['sha256']
                and admission['complete'] is True
                and admission['expected_repaired'] is True
                and all(not row['accepted'] for row in admission['controls'].values()),
                'installed actual comparator admission')

    def validate_source(self, source=None):
        s = self.source if source is None else source
        r = self.registry
        require(s['complete'] is True and s['full_goal_accepted'] is False
                and s['observations'] == 8732 and len(s['original_tests']) == 19,
                'bounded complete original source replay')
        require(len(s['steps']) == 9 and all(type(step['exit_code']) is int
                and step['exit_code'] == 0 for step in s['steps']),
                'all original/native/imported source steps')
        require(s['steps'][1]['command'][:4] == ['cargo', 'test', '--offline', '--locked']
                and s['steps'][2]['command'][:4] == ['cargo', 'build', '--offline', '--locked'],
                'locked original Rust tests and build')
        q = self.acceptance
        require(q['complete'] is True and q['whole_source_accepted'] is True
                and q['full_goal_accepted'] is False
                and q['source_observations'] == 8732
                and q['retained_observations'] == 6532
                and q['new_observations'] == 2200
                and len(q['original_tests']) == 19
                and q['evidence_sha256'][r['source_execution']['path']]
                == r['source_execution']['sha256'],
                'terminal complete source acceptance and evidence identity')
        require(len(s['matrices']) == 2, 'complete original matrix selection')
        for matrix, (name, corpus_path, count, index, corpus_sha) in zip(
                s['matrices'], (
                    ('retained', r['corpus'], 6532, 3, s['retained_corpus_sha256']),
                    ('source', r['additional_corpus'], 2200, 6, s['corpus_sha256']))):
            corpus = read(ROOT / corpus_path)
            require(digest(ROOT / corpus_path) == corpus_sha,
                    'unchanged complete source corpus: ' + name)
            require(corpus['baseline'] == r['runtime_commit']
                    and len(corpus['cases']) == count
                    and all(type(row) is dict and set(row) == {'input', 'expected'}
                            for row in corpus['cases']), 'exact retained/source case rows')
            expected = [row['expected'] for row in corpus['cases']]
            require(matrix['name'] == name and matrix['observations'] == count
                    and same(matrix['inputs'], [row['input'] for row in corpus['cases']])
                    and matrix['outputs']['original']['step'] == index
                    and same(json_lines(s['steps'][index]['stdout']), expected)
                    and matrix['differences'] == [], 'complete raw original stream: ' + name)

    def validate_commands(self, tests, build):
        r = self.registry
        by_name = {test['name']: test for test in tests}
        require(len(by_name) == len(tests), 'unique CTest names')
        script = str(ROOT / r['comparator'])
        require({test['name'] for test in tests if len(test.get('command', [])) > 1
                 and test['command'][1] == script} == {r['test']},
                'complete actual in-memory CLI selection')
        test = by_name[r['test']]
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [script, '--cpp',
                                           str(build.resolve() / r['binary']),
                                           '--cpp-emulator='],
                'exact current in-memory CTest command')
        props = {property['name']: property['value'] for property in test.get('properties', [])}
        require('inmemory_cli_source' in props.get('FIXTURES_REQUIRED', []),
                'required complete in-memory source fixture')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        r = self.registry
        binary = build.resolve() / r['binary']
        bits = subprocess.check_output([str(binary), '--pointer-bits'], text=True).strip()
        require(bits == '64', 'historical in-memory corpus is native64-only')
        corpus = read(ROOT / r['corpus'])
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        actual = observe(str(binary), inputs)
        require(same(actual, expected), 'complete current in-memory native results')
        return dict(count=len(actual), binary_sha256=digest(binary))


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
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    validator = Validator(read(args.contracts))
    validator.validate_files()
    validator.validate_source()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      accepted_original_source_observations=8732,
                      current_native_observations=native['count'] if native else 0,
                      native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('Verified complete retained in-memory source: 8732 original outcomes'
          + (' and 6532 fresh current native outputs' if native else ''))


if __name__ == '__main__':
    main()
