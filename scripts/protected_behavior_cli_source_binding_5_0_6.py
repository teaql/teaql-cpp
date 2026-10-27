#!/usr/bin/env python3
"""Bind protected Behavior CLI to complete original Rust source execution."""

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
CONTRACT = ROOT / 'tests/parity/protected_behavior_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from build_protected_behavior_oracle_5_0_6 import BRIDGE
from compare_protected_behavior_5_0_6 import cases
from compare_values import observe, same
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


def archived_original_provenance(commit):
    archive = subprocess.check_output(
        ['git', '-C', str(ROOT / '.reference/teaql-rs'), 'archive', commit])
    records = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if not member.isfile():
                continue
            original = stream.extractfile(member).read()
            suffix = (BRIDGE.encode()
                      if member.name == 'teaql-runtime/src/data_service/resolved.rs'
                      else b'')
            records[member.name] = dict(
                original_sha256=digest_bytes(original),
                compiled_sha256=digest_bytes(original + suffix),
                appended_bytes=len(suffix))
    return dict(runtime_commit=commit, bridge=BRIDGE, files=records,
                notice='Complete original archive remains a byte-identical '
                       'prefix; the bridge only calls original private methods.')


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
            require(actual == wanted, 'locked original checkout: ' + name)
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus', 'original_receipt', 'historical_source',
                    'historical_terminal'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)

        source = read(ROOT / contract['historical_source'])
        terminal = read(ROOT / contract['historical_terminal'])
        require(source['runtime_commit'] == terminal['runtime_commit']
                == contract['runtime_commit']
                and source['original_source_files']
                == terminal['original_source_files']
                == contract['original_source_files']
                and source['appended_source_files']
                == terminal['appended_source_files'] == 1
                and source['original_method_bodies_changed'] is False
                and terminal['original_method_bodies_changed'] is False
                and terminal['observations'] == contract['count']
                and terminal['normalization'] == 'none',
                'historical protected source boundary')
        for historical in (source, terminal):
            stale = [path for path, wanted in historical['hashes'].items()
                     if digest(ROOT / path) != wanted]
            require(stale == [contract['historical_comparator_path']],
                    'single historical comparator split boundary')

        provenance = archived_original_provenance(contract['runtime_commit'])
        require(len(provenance['files']) == contract['original_source_files']
                and provenance['bridge'] == source['bridge'],
                'complete byte-identical original archive and one access bridge')
        adapted = [path for path, row in provenance['files'].items()
                   if row['appended_bytes']]
        require(adapted == ['teaql-runtime/src/data_service/resolved.rs'],
                'only private-method access bridge')
        resolved = provenance['files'][adapted[0]]
        require(resolved['original_sha256'] == source['original_resolved_sha256']
                and resolved['compiled_sha256'] == source['compiled_resolved_sha256'],
                'original and compiled protected service source')

        generated_path = ROOT / contract['generated_provenance']
        generated = read(generated_path)
        require(generated['runtime_commit'] == contract['runtime_commit']
                and generated['generator_commit'] == contract['generator_commit']
                and generated['hand_edited'] is False
                and len(generated['files']) == contract['generated_files'],
                'complete original generated Checker domain')
        require(digest(ROOT / generated['model']) == generated['model_sha256'],
                'original Checker model')
        for path, wanted in generated['files'].items():
            require(digest(generated_path.parent / path) == wanted,
                    'original generated Checker file: ' + path)

        observer = ROOT / 'tests/parity/rust_protected_behavior_5_0_6/Cargo.lock'
        reference = ROOT / '.reference/teaql-rs/Cargo.lock'
        require(registry_identities(observer) <= registry_identities(reference),
                'reference-seeded observer registry closure')
        wanted_versions = versions(reference)
        local = {'teaql_protected_behavior_506', 'cpp-checker-fixture-core'}
        for package, values in versions(observer).items():
            if package not in local:
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded observer version: ' + package)

        original = read(ROOT / contract['original_receipt'])
        require(original['complete'] is True
                and original['runtime_commit'] == contract['runtime_commit']
                and original['generator_commit'] == contract['generator_commit']
                and original['build_exit_code'] == 0
                and original['comparison_exit_code'] == 0
                and original['count'] == contract['count']
                and original['temporary_target_removed'] is True,
                'fresh complete original Rust execution receipt')
        for recorded, path in (
                ('source_manifest_sha256',
                 'tests/parity/rust_protected_behavior_5_0_6/Cargo.toml'),
                ('source_lock_sha256',
                 'tests/parity/rust_protected_behavior_5_0_6/Cargo.lock'),
                ('source_observer_sha256',
                 'tests/parity/protected_behavior_5_0_6.rs'),
                ('generated_checker_provenance_sha256',
                 contract['generated_provenance'])):
            require(original[recorded] == digest(ROOT / path),
                    'fresh protected source: ' + path)
        provenance_bytes = (json.dumps(provenance, indent=2) + '\n').encode()
        require(original['derived_provenance_sha256']
                == digest_bytes(provenance_bytes),
                'fresh complete original archive provenance')
        corpus = read(ROOT / contract['corpus'])
        require(same(original['original_outputs'],
                     [row['expected'] for row in corpus['cases']]),
                'complete fresh original protected outcomes')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['baseline'] == self.contract['runtime_commit']
                and len(corpus['cases']) == self.contract['count'],
                'complete protected source corpus')
        require(same([row['input'] for row in corpus['cases']], list(cases())),
                'complete protected input matrix')
        outputs = [row['expected'] for row in corpus['cases']]
        require(same(outputs, [row['expected'] for row in retained['cases']]),
                'unchanged complete protected outcomes')
        stats = dict(behavior_callbacks=sum(
                         call['stage'] == 'behavior' for row in outputs
                         for call in row['calls']),
                     request_policy_callbacks=sum(
                         call['stage'] == 'policy' for row in outputs
                         for call in row['calls']),
                     raw_events=sum(len(row['events']) for row in outputs),
                     sql_log_records=sum(len(row['logs']) for row in outputs),
                     successful_return_counts=sum(
                         'rows' in row['outcome'] for row in outputs))
        terminal = read(ROOT / self.contract['historical_terminal'])
        require(stats == {key: terminal[key] for key in stats},
                'complete Behavior, Policy, event, SQL and result boundaries')
        return corpus

    def validate_commands(self, tests, build):
        script = str(ROOT / self.contract['comparator'])
        selected = [row for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script]
        require(len(selected) == 1
                and selected[0]['name'] == self.contract['comparison'],
                'unique installed protected Behavior comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / self.contract['probe'])],
                'exact installed protected Behavior CTest command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('protected_behavior_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed protected Behavior comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = self.validate_corpus()
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / self.contract['probe']
        actual = observe(binary, inputs)
        require(len(actual) == self.contract['count'] and same(actual, expected),
                'fresh complete protected Behavior results')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary), raw_autocommit_only=True)


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
    print('Verified protected Behavior source: 1296 Rust outcomes'
          + (' / 1296 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
