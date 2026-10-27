#!/usr/bin/env python3
"""Bind the installed generated member-alias comparison to its Rust source."""

import argparse
import copy
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/generated_member_alias_cli_contract_5_0_6.json'
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
    aliases = ('type', 'type_field', 'legacy_type', 'type_typo')
    for model in ('record', 'child'):
        for length in range(len(aliases) + 1):
            for names in itertools.permutations(aliases, length):
                for null in (False, True):
                    yield dict(model=model, names=list(names), null=null)


class Validator:
    def __init__(self, contract):
        self.contract = contract

    def validate_files(self):
        contract = self.contract
        require(read(ROOT / 'upstream.lock.json')['commit'] == contract['runtime_commit'],
                'locked Rust runtime')
        for name, wanted in (('teaql-rs', contract['runtime_commit']),
                             ('teaql_code_gen', contract['generator_commit'])):
            actual = subprocess.check_output(
                ['git', '-C', str(ROOT / '.reference' / name), 'rev-parse', 'HEAD'],
                text=True).strip()
            require(actual == wanted, 'locked checkout: ' + name)
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus', 'source_receipt'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)

        corpus = read(ROOT / contract['corpus'])
        for path, wanted in corpus['source_hashes'].items():
            require(digest(ROOT / path) == wanted, 'original source: ' + path)
        provenance_path = ROOT / contract['provenance']
        provenance = read(provenance_path)
        require(provenance['runtime_commit'] == contract['runtime_commit']
                and provenance['generator_commit'] == contract['generator_commit']
                and provenance['hand_edited'] is False
                and len(provenance['files']) == contract['generated_files'],
                'complete original generated domain')
        require(digest(ROOT / provenance['model']) == provenance['model_sha256'],
                'original model input')
        for path, wanted in provenance['files'].items():
            require(digest(provenance_path.parent / path) == wanted,
                    'original generated file: ' + path)

        receipt = read(ROOT / contract['source_receipt'])
        require(receipt['complete'] is True
                and receipt['actual_original_replayed'] is True
                and receipt['observations'] == contract['count']
                and receipt['corpus_sha256'] == contract['corpus_sha256']
                and receipt['source_hashes'] == corpus['source_hashes'],
                'retained original source execution')
        originals = [step for step in receipt['steps'] if step['label'] == 'original']
        require(len(originals) == 1 and originals[0]['exit_code'] == 0,
                'one successful original Rust execution')
        require(same(json_lines(originals[0]['stdout']), corpus['expected']),
                'complete retained original Rust observations')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['runtime_commit'] == self.contract['runtime_commit']
                and len(corpus['inputs']) == len(corpus['expected']) == self.contract['count'],
                'complete source corpus')
        require(same(corpus['inputs'], list(matrix())),
                'complete ordered alias input matrix')
        require(same(corpus['expected'], retained['expected']),
                'unchanged complete original outcomes')
        require(all(set(row) == {'dirty', 'loaded', 'new', 'value'}
                    and set(row['loaded']) == {'id', 'version', 'type', 'type_field',
                                               'legacy_type', 'type_typo'}
                    for row in corpus['expected']),
                'whole generated model observations')
        return corpus

    def validate_commands(self, tests, build):
        contract = self.contract
        script = str(ROOT / contract['comparator'])
        selected = [row for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script]
        require(len(selected) == 1 and selected[0]['name'] == contract['comparison'],
                'unique installed source comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / contract['probe'])],
                'exact installed CTest command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('generated_member_alias_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture required by actual comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        corpus = self.validate_corpus()
        binary = build.resolve() / self.contract['probe']
        payload = ''.join(json.dumps(row) + '\n' for row in corpus['inputs'])
        process = subprocess.run([str(binary)], input=payload, text=True,
                                 capture_output=True,
                                 env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'),
                                 check=False)
        require(process.returncode == 0, 'current C++ observer execution')
        actual = json_lines(process.stdout)
        require(len(actual) == self.contract['count']
                and same(actual, corpus['expected']),
                'fresh complete generated member-alias observations')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary))


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
                       source_observations=validator.contract['count'],
                       native=native, registry_sha256=digest(args.contracts),
                       original_rust_rerun=False)
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Verified generated member aliases: 260 original source observations'
          + (' / 260 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
