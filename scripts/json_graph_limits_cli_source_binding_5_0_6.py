#!/usr/bin/env python3
"""Bind installed JSON graph-limit CLI to complete original Rust evidence."""

import argparse
import copy
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'tests/parity/json_graph_limits_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from build_private_save_oracles_5_0_6 import APPENDS, FAMILIES, PROJECT
from compare_values import observe, same
from replay_json_graph_limits_5_0_6 import inputs
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def read(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def digest_bytes(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def archived_private_provenance(commit):
    archive = subprocess.check_output(
        ['git', '-C', str(ROOT / '.reference/teaql-rs'), 'archive', commit])
    records = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if not member.isfile():
                continue
            original = stream.extractfile(member).read()
            appended = APPENDS.get(member.name, '').encode()
            records[member.name] = dict(
                original_sha256=digest_bytes(original),
                compiled_sha256=digest_bytes(original + appended),
                appended_bytes=len(appended))
    observers = {family: digest(PROJECT / 'src/bin' /
                                (family + '_5_0_6.rs')) for family in FAMILIES}
    return dict(schema=1, runtime_commit=commit,
                notice='Original archive bytes remain intact; only the listed '
                       'test access bridges are appended. This is source-build '
                       'provenance, not semantic acceptance.',
                appended_bridges=APPENDS, files=records, observers=observers,
                build_status='prepared')


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
        require(actual == contract['runtime_commit'], 'locked original checkout')
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus', 'original_receipt'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)

        provenance = archived_private_provenance(contract['runtime_commit'])
        require(len(provenance['files']) == contract['original_source_files']
                and len(provenance['appended_bridges'])
                == contract['bridge_targets']
                and set(provenance['appended_bridges']) <= set(provenance['files']),
                'complete original archive and explicit access bridges')
        for path, appended in provenance['appended_bridges'].items():
            require(provenance['files'][path]['appended_bytes']
                    == len(appended.encode()),
                    'exact append-only access bridge: ' + path)

        observer = ROOT / 'tests/parity/rust_private_save_5_0_6/Cargo.lock'
        reference = ROOT / '.reference/teaql-rs/Cargo.lock'
        require(registry_identities(observer) <= registry_identities(reference),
                'reference-seeded observer registry closure')
        wanted_versions = versions(reference)
        for package, values in versions(observer).items():
            if package != 'teaql_cpp_private_save_oracle':
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded observer version: ' + package)

        original = read(ROOT / contract['original_receipt'])
        require(original['complete'] is True
                and original['runtime_commit'] == contract['runtime_commit']
                and original['build_exit_code'] == 0
                and original['comparison_exit_code'] == 0
                and original['count'] == contract['count']
                and original['temporary_target_removed'] is True
                and original['existing_derived_source_preserved'] is True,
                'fresh original JSON graph execution receipt')
        for recorded, path in (
                ('source_manifest_sha256',
                 'tests/parity/rust_private_save_5_0_6/Cargo.toml'),
                ('source_lock_sha256',
                 'tests/parity/rust_private_save_5_0_6/Cargo.lock'),
                ('source_driver_sha256', contract['driver'])):
            require(original[recorded] == digest(ROOT / path),
                    'fresh original source: ' + path)
        provenance_bytes = (json.dumps(provenance, indent=2) + '\n').encode()
        require(original['derived_provenance_sha256']
                == digest_bytes(provenance_bytes),
                'fresh complete original archive provenance')
        corpus = read(ROOT / contract['corpus'])
        require(same(original['original_outputs'],
                     [row['expected'] for row in corpus['cases']]),
                'complete fresh original JSON graph outcomes')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['baseline'] == self.contract['runtime_commit']
                and corpus['version'] == '5.0.6'
                and corpus['driver'] == self.contract['driver']
                and corpus['driver_sha256'] == digest(ROOT / corpus['driver'])
                and len(corpus['cases']) == self.contract['count'],
                'complete original graph-limit corpus')
        require(same([row['input'] for row in corpus['cases']], list(inputs())),
                'complete eleven-case graph input matrix')
        require(same([row['expected'] for row in corpus['cases']],
                     [row['expected'] for row in retained['cases']]),
                'unchanged complete graph-limit outcomes')
        require(all(set(row['expected']) in ({'error'}, {'row'})
                    for row in corpus['cases']),
                'whole graph-limit result shape')
        return corpus

    def validate_commands(self, tests, build):
        script = str(ROOT / self.contract['comparator'])
        selected = [row for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script]
        require(len(selected) == 1
                and selected[0]['name'] == self.contract['comparison'],
                'unique installed graph-limit comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / self.contract['probe']),
                                          '--cpp-emulator='],
                'exact installed graph-limit CTest command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('json_graph_limits_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed graph-limit comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = self.validate_corpus()
        stimuli = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / self.contract['probe']
        actual = observe(binary, stimuli, arguments=['--large-fixture'])
        require(len(actual) == self.contract['count'] and same(actual, expected),
                'fresh complete graph-limit results')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary), large_fixture=True)


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
                       fresh_original_observations=validator.contract['count'],
                       native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Verified original JSON graph limits: 11 Rust outcomes'
          + (' / 11 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
