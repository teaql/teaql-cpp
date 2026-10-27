#!/usr/bin/env python3
"""Bind installed SQLite runtime examples to original Rust 5.0.6 execution."""

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
CONTRACT = ROOT / 'tests/parity/runtime_examples_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_values import same
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def read(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


class Validator:
    def __init__(self, contract):
        self.contract = contract

    def validate_files(self):
        contract = self.contract
        require(read(ROOT / 'upstream.lock.json')['commit'] == contract['runtime_commit'],
                'locked runtime')
        reference = ROOT / '.reference/teaql-rs'
        head = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                       text=True).strip()
        dirty = subprocess.check_output(['git', '-C', str(reference), 'status',
                                         '--porcelain', '--untracked-files=no'], text=True)
        require(head == contract['runtime_commit'] and not dirty,
                'unchanged original source checkout')
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for path, wanted in contract['evidence_files'].items():
            require(digest(ROOT / path) == wanted, 'retained evidence: ' + path)
        reference_lock = reference / 'Cargo.lock'
        observer_lock = ROOT / contract['source_lock']
        require(registry_identities(observer_lock) <= registry_identities(reference_lock),
                'reference-seeded registry identities')
        resolved = versions(observer_lock)
        require(len(resolved) == contract['dependency_packages'],
                'complete narrow example observer closure')
        wanted_versions = versions(reference_lock)
        for package, values in resolved.items():
            if package != 'teaql_cpp_runtime_examples_source_506':
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded dependency version: ' + package)
        corpus = self.validate_corpus()
        provenance = self.validate_provenance()
        original = read(ROOT / contract['original_receipt'])
        require(original['complete'] is True
                and original['whole_goal_complete'] is False
                and original['runtime_commit'] == contract['runtime_commit']
                and original['build_exit_code'] == 0
                and original['comparison_exit_code'] == 0
                and original['temporary_target_removed'] is True,
                'fresh original example execution')
        for field, path in (('source_manifest_sha256', contract['source_manifest']),
                            ('source_lock_sha256', contract['source_lock']),
                            ('reference_lock_sha256', '.reference/teaql-rs/Cargo.lock'),
                            ('source_driver_sha256', contract['driver'])):
            require(original[field] == digest(ROOT / path),
                    'original source input: ' + path)
        inputs = [case['input'] for case in corpus['cases']]
        require(len(original['business_outputs']) == contract['count']
                and same(original['business_outputs'],
                         [case['expected'] for case in corpus['cases']]),
                'complete original business outputs')
        logs = [provenance[item['mode']] for item in inputs]
        require(len(original['sql_provenance']) == contract['count']
                and same(original['sql_provenance'], logs)
                and sum(map(len, logs)) == contract['sql_entries'],
                'complete ordered original SQL provenance')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['baseline'] == self.contract['runtime_commit']
                and len(corpus['cases']) == self.contract['count']
                and [case['input'] for case in corpus['cases']]
                == [{'mode': 'schema'}, {'mode': 'relations'}]
                and same(corpus['cases'], retained['cases']),
                'complete original business scenario corpus')
        return corpus

    def validate_provenance(self, candidate=None):
        contract = self.contract
        corpus = read(ROOT / contract['corpus'])
        retained = read(ROOT / contract['provenance'])
        provenance = copy.deepcopy(candidate) if candidate is not None else retained
        hashes = {path: digest(ROOT / path) for path in contract['original_sources']}
        require(corpus['source_hashes'] == hashes
                and provenance['source_hashes'] == hashes
                and provenance['baseline'] == contract['runtime_commit']
                and same(provenance, retained)
                and set(provenance) == {'baseline', 'note', 'source_hashes',
                                       'schema', 'relations'}
                and sum(map(len, (provenance['schema'], provenance['relations'])))
                == contract['sql_entries'],
                'complete original source and ordered SQL provenance')
        return provenance

    def validate_commands(self, tests, build):
        contract = self.contract
        script = str(ROOT / contract['comparator'])
        selected = [test for test in tests
                    if len(test.get('command', [])) > 1
                    and test['command'][1] == script]
        require(len(selected) == 1 and selected[0]['name'] == contract['comparison'],
                'unique installed runtime examples comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [
                    script, '--schema-crud',
                    str(build.resolve() / 'teaql_sqlite_schema_crud'),
                    '--relations-graph',
                    str(build.resolve() / 'teaql_sqlite_relations_graph'),
                    '--cpp-emulator='],
                'exact installed runtime examples comparison command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('runtime_examples_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed runtime examples comparison')
        require('TEAQL_LOG_ENDPOINT=off' in props.get('ENVIRONMENT', []),
                'installed example log environment')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        corpus = self.validate_corpus()
        environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off')
        outputs = []
        binaries = {}
        for name, prefix in (('teaql_sqlite_schema_crud', 'schema+crud example rows: '),
                             ('teaql_sqlite_relations_graph',
                              'relation+graph example rows: ')):
            binary = build.resolve() / name
            result = subprocess.run([str(binary)], text=True, capture_output=True,
                                    check=True, env=environment)
            rows = [line[len(prefix):] for line in result.stdout.split('\n')
                    if line.startswith(prefix)]
            require(len(rows) == 1, 'one complete example row: ' + name)
            outputs.append(json.loads(rows[0]))
            binaries[name] = digest(binary)
        require(len(outputs) == self.contract['count']
                and same(outputs, [case['expected'] for case in corpus['cases']]),
                'fresh complete native example results')
        return dict(observations=len(outputs), exact_equalities=len(outputs),
                    binary_sha256=binaries)


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
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      fresh_original_scenarios=validator.contract['count'],
                      original_sql_entries=validator.contract['sql_entries'],
                      native=native, contract_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(), mtime=0))
    print('Verified original runtime examples: 2 business outcomes / 34 SQL entries'
          + (' / 2 fresh exact C++ examples' if native else ''))


if __name__ == '__main__':
    main()
