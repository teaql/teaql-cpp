#!/usr/bin/env python3
"""Bind the installed Indexed LoadState comparison to original Rust execution."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/eval_indexed_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same


def read(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def matrix():
    kinds = ('NotLoaded', 'Partial', 'PartialCompact', 'SharedColumns',
             'Indexed', 'FullyLoaded')
    fields = ((), ('id',), ('id', 'version', 'sql_2', 'sql_64', 'sql_129',
                           'lines', '#memo', 'unknown'))
    actions = tuple(dict(action=action, field=field) for action, field in (
        ('load', 'id'), ('load', 'sql_64'), ('load', 'field_129'),
        ('load', 'lines'), ('load', '#memo'), ('load', 'unknown'),
        ('unload', 'id'), ('unload', 'lines'), ('unload', '#memo'),
        ('unload', 'unknown')))
    for kind in kinds:
        for names in fields:
            yield dict(kind=kind, fields=list(names), actions=list(actions))


class Validator:
    def __init__(self, contract):
        self.contract = contract

    def validate_files(self):
        contract = self.contract
        require(read(ROOT / 'upstream.lock.json')['commit'] == contract['runtime_commit'],
                'locked Rust runtime')
        actual = subprocess.check_output(
            ['git', '-C', str(ROOT / '.reference/teaql-rs'), 'rev-parse', 'HEAD'],
            text=True).strip()
        require(actual == contract['runtime_commit'], 'locked original Rust checkout')
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus', 'source_receipt'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        corpus = read(ROOT / contract['corpus'])
        for path, wanted in corpus['source_hashes'].items():
            require(digest(ROOT / path) == wanted, 'original source: ' + path)
        receipt = read(ROOT / contract['source_receipt'])
        require(receipt['complete'] is True
                and receipt['source_count'] == contract['count']
                and receipt['count'] == contract['count'],
                'complete historical original execution')
        originals = [step for step in receipt['steps'] if step['label'] == 'original']
        require(len(originals) == 1 and originals[0]['exit_code'] == 0,
                'one successful original Rust execution')
        require(same(json_lines(originals[0]['stdout']), corpus['expected']),
                'whole retained Rust output and frozen source corpus')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['runtime_commit'] == self.contract['runtime_commit']
                and len(corpus['inputs']) == len(corpus['expected']) == self.contract['count'],
                'complete Indexed source corpus')
        require(same(corpus['inputs'], list(matrix())),
                'complete six-state/three-layout input matrix')
        require(same(corpus['expected'], retained['expected']),
                'unchanged complete original Indexed observations')
        require(all('common' in entry
                    and 'initial_diagnostics' in entry
                    and 'final_diagnostics' in entry
                    for entry in corpus['expected']),
                'whole original Indexed result shape')
        return corpus

    def validate_commands(self, tests, build):
        contract = self.contract
        script = str(ROOT / contract['comparator'])
        selected = [row for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script]
        require(len(selected) == 1 and selected[0]['name'] == contract['comparison'],
                'unique installed Indexed comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / contract['probe']),
                                          '--cpp-emulator='],
                'exact installed Indexed CTest command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('eval_indexed_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed Indexed comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        corpus = self.validate_corpus()
        binary = build.resolve() / self.contract['probe']
        payload = ''.join(json.dumps(row) + '\n' for row in corpus['inputs'])
        process = subprocess.run([str(binary)], input=payload, text=True,
                                 capture_output=True,
                                 env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'),
                                 check=False)
        require(process.returncode == 0, 'current Indexed observer execution')
        actual = json_lines(process.stdout)
        require(len(actual) == self.contract['count']
                and same([entry['common'] for entry in actual],
                         [entry['common'] for entry in corpus['expected']]),
                'fresh complete common Indexed semantics')
        for entry in actual:
            for name in ('initial_diagnostics', 'final_diagnostics'):
                diagnostics = entry[name]
                require(len(diagnostics['actual']) == 4
                        and all(diagnostics['actual'])
                        and diagnostics['complete'] is True,
                        'four complete initialized diagnostics')
                if diagnostics['expected_snapshot_wrapper']:
                    require(same(diagnostics['actual'],
                                 diagnostics['expected_snapshot_wrapper']),
                            'snapshot diagnostic wrapper')
        return dict(scenarios=len(actual), mutations=10 * len(actual),
                    exact_common_equalities=len(actual), binary_sha256=digest(binary),
                    whole_outputs_equal=same(actual, corpus['expected']))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, default=CONTRACT)
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
    validator.validate_corpus()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        receipt = dict(complete=True, whole_goal_complete=False,
                       source_scenarios=validator.contract['count'],
                       native=native, registry_sha256=digest(args.contracts),
                       original_rust_rerun=False)
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Verified Indexed source: 18 original scenarios'
          + (' / 18 fresh common C++ results' if native else ''))


if __name__ == '__main__':
    main()
