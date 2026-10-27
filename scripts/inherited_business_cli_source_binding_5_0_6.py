#!/usr/bin/env python3
"""Bind inherited Business-ID CLI to its explicit original-source bridge."""

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
CONTRACT = ROOT / 'tests/parity/inherited_business_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_inherited_business_extensions_5_0_6 import cases
from compare_values import observe, same
from build_inherited_business_binding_bridge_5_0_6 import CHILDREN
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
                and source['generator_commit'] == terminal['generator_commit']
                == contract['generator_commit']
                and source['adapted_source_only'] is True
                and source['untouched_producer_acceptance'] is False
                and terminal['observations'] == contract['count']
                and terminal['normalization'] == 'none'
                and terminal['source_artifact_acceptance'] is False,
                'historical adapted source boundary')
        for historical in (source, terminal):
            stale = [path for path, wanted in historical['hashes'].items()
                     if digest(ROOT / path) != wanted]
            require(stale == [contract['historical_comparator_path']],
                    'single historical comparator split boundary')

        provenance_path = ROOT / contract['provenance']
        provenance = read(provenance_path)
        bridge = source['derived_provenance']
        require(provenance['runtime_commit'] == bridge['runtime_commit']
                == contract['runtime_commit']
                and provenance['generator_commit'] == bridge['generator_commit']
                == contract['generator_commit']
                and provenance['hand_edited'] is False
                and bridge['original_files_unchanged'] is True
                and bridge['untouched_producer_acceptance'] is False
                and len(provenance['files']) == len(bridge['files'])
                == contract['generated_files']
                and bridge['original_files_unchanged'] is True,
                'complete original and adapted generated domain')
        require(digest(ROOT / provenance['model']) == provenance['model_sha256']
                and bridge['model_sha256'] == provenance['model_sha256'],
                'original inherited Business-ID model')
        require(set(provenance['files']) == {'lib/' + path for path in bridge['files']},
                'complete bridge file identity')
        original_root = provenance_path.parent
        parent = (original_root / 'lib/src/document/entity.rs').read_text()
        slot = parent[parent.index('pub(crate) struct DocumentDocumentNumberBusinessIdSlot'):]
        require(digest_bytes(slot.encode()) == bridge['slot_body_sha256'],
                'original parent Business-ID slot body')
        for path, record in bridge['files'].items():
            original_bytes = (original_root / 'lib' / path).read_bytes()
            require(digest_bytes(original_bytes)
                    == record['original_sha256']
                    == provenance['files']['lib/' + path],
                    'original generated file: ' + path)
            adapted = original_bytes
            prefix = b''
            suffix = b''
            if path == 'src/document/entity.rs':
                declaration = b'pub struct DocumentNumber(String);'
                require(original_bytes.count(declaration) == 1,
                        'original shared identifier declaration')
                adapted = adapted.replace(
                    declaration, b'pub struct DocumentNumber(pub(crate) String);', 1)
            for folder, entity in CHILDREN.items():
                if path == 'src/' + folder + '/entity.rs':
                    prefix = b'use crate::document::DocumentNumber;\n'
                    declaration = "pub(crate) &'a mut Document,"
                    require(slot.count(declaration) == 1,
                            'original parent tuple carrier')
                    suffix = ('\n' + slot.replace(
                        declaration, f"pub(crate) &'a mut {entity},", 1)).encode()
            require(digest_bytes(prefix + adapted + suffix)
                    == record['adapted_sha256']
                    and len(prefix) == record['prefixed_bytes']
                    and len(suffix) == record['appended_bytes'],
                    'explicit derived bridge file: ' + path)
        bridged = [path for path, record in bridge['files'].items()
                   if record['appended_bytes']]
        visible = [path for path, record in bridge['files'].items()
                   if record['crate_private_tuple_visibility_bridge']]
        require(len(bridged) == 4
                and visible == ['src/document/entity.rs'],
                'exact four-child binding and one visibility bridge')

        observer = ROOT / 'tests/parity/rust_inherited_business_extensions_5_0_6/Cargo.lock'
        reference = ROOT / '.reference/teaql-rs/Cargo.lock'
        require(registry_identities(observer) <= registry_identities(reference),
                'reference-seeded observer registry closure')
        wanted_versions = versions(reference)
        local = {'teaql_inherited_business_extensions_506',
                 'cpp-inheritance-business-ids-core'}
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
                'fresh adapted original execution receipt')
        for recorded, path in (
                ('source_manifest_sha256',
                 'tests/parity/rust_inherited_business_extensions_5_0_6/Cargo.toml'),
                ('source_lock_sha256',
                 'tests/parity/rust_inherited_business_extensions_5_0_6/Cargo.lock'),
                ('source_observer_sha256',
                 'tests/parity/inherited_business_extensions_5_0_6.rs'),
                ('original_generated_provenance_sha256', contract['provenance'])):
            require(original[recorded] == digest(ROOT / path),
                    'fresh adapted source: ' + path)
        bridge_bytes = (json.dumps(bridge, indent=2) + '\n').encode()
        require(original['bridge_provenance_sha256'] == digest_bytes(bridge_bytes),
                'fresh explicit bridge provenance')
        corpus = read(ROOT / contract['corpus'])
        require(same(original['original_outputs'],
                     [row['expected'] for row in corpus['cases']]),
                'complete fresh adapted original outcomes')

    def validate_corpus(self, candidate=None):
        retained = read(ROOT / self.contract['corpus'])
        corpus = copy.deepcopy(candidate) if candidate is not None else retained
        require(corpus['baseline'] == self.contract['runtime_commit']
                and corpus['generator_commit'] == self.contract['generator_commit']
                and corpus['source_artifact_acceptance'] is False
                and len(corpus['cases']) == self.contract['count'],
                'complete adapted source corpus')
        require(same([row['input'] for row in corpus['cases']], list(cases())),
                'complete inherited Business-ID input matrix')
        outputs = [row['expected'] for row in corpus['cases']]
        require(same(outputs, [row['expected'] for row in retained['cases']]),
                'unchanged complete inherited outcomes')
        stats = dict(callbacks=sum(len(row['hooks']) for row in outputs),
                     policy_reviews=sum(len(row['plans']) for row in outputs),
                     findings=sum(len(row['diagnostics']) for row in outputs),
                     fix_evidence=sum(len(row['fixes']) for row in outputs),
                     successful_saves=sum(row['saved'] is not None for row in outputs))
        terminal = read(ROOT / self.contract['historical_terminal'])
        require(stats == {key: terminal[key] for key in stats},
                'complete Checker, Policy, Fix and Save boundaries')
        return corpus

    def validate_commands(self, tests, build):
        script = str(ROOT / self.contract['comparator'])
        selected = [row for row in tests
                    if len(row.get('command', [])) > 1 and row['command'][1] == script]
        require(len(selected) == 1
                and selected[0]['name'] == self.contract['comparison'],
                'unique installed inherited business comparison')
        row = selected[0]
        require(Path(row['command'][0]).resolve() == Path(sys.executable).resolve()
                and row['command'][1:] == [script, '--cpp',
                                          str(build.resolve() / self.contract['probe'])],
                'exact installed inherited business CTest command')
        props = {item['name']: item['value'] for item in row.get('properties', [])}
        require('inherited_business_cli' in props.get('FIXTURES_REQUIRED', []),
                'source fixture on installed inherited business comparison')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
            text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ.update(TEAQL_LOG_ENDPOINT='off', TEAQL_BUSINESS_ID_KEY_VERSION='1',
                          TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
        corpus = self.validate_corpus()
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / self.contract['probe']
        actual = observe(binary, inputs)
        require(len(actual) == self.contract['count'] and same(actual, expected),
                'fresh complete adapted inherited business results')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary), adapted_source_only=True)


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
                       fresh_adapted_original_observations=validator.contract['count'],
                       native=native, registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Verified inherited Business-ID bridge: 450 adapted Rust outcomes'
          + (' / 450 fresh exact C++ results' if native else ''))


if __name__ == '__main__':
    main()
