#!/usr/bin/env python3
"""Compare the scoped configuration kernel with actual original observations.

This covers configuration parsing and the two NUL preparation failures, not
complete generated runtime assembly or ordinary successful SQLite connections.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same
from verify_constant_closure_5_0_6 import registry_identities


SOURCE_RECORDS = {
    'runtime_configuration_original_observations_5_0_6.json': (
        'e34014a570b1029512182795262006f0c1205367151ddbbb1dc7a787db3045d9',
        ('tests/generator/parity/dynamic_json_rust/lib/src/runtime.rs',
         'tests/generator/parity/dynamic_json_rust/PROVENANCE.json',
         'tests/parity/rust_execution_shapes_5_0_6/Cargo.lock',
         'tests/parity/rust_execution_shapes_5_0_6/src/bin/runtime_configuration_original_5_0_6.rs')),
    'runtime_configuration_nul_original_observations_5_0_6.json': (
        'c5b0abb63c32caa09e1ab55ab928cde35a466f79eec2d84a9ef55bb499d1d30a',
        ('tests/parity/rust_execution_shapes_5_0_6/src/bin/runtime_configuration_nul_original_5_0_6.rs',
         'tests/generator/parity/dynamic_json_rust/lib/src/runtime.rs',
         'tests/parity/rust_execution_shapes_5_0_6/Cargo.lock')),
    'runtime_service_errors_original_observations_5_0_6.json': (
        '1d29dded225d78be36ad0565edbaf0501721b81673c61e6878c74a3afa1a8378',
        ('tests/parity/rust_execution_shapes_5_0_6/src/bin/runtime_service_errors_original_5_0_6.rs',
         'tests/generator/parity/dynamic_json_rust/lib/src/runtime.rs',
         'tests/parity/rust_execution_shapes_5_0_6/Cargo.lock')),
    'generated_registry_schema_original_5_0_6.json': (
        '8f7e658cd9262525d109c44e923f6064c182af6a3d68c47081ff454ec4115d30',
        ('tests/parity/rust_runtime_configuration_models_5_0_6/src/bin/registries.rs',
         'tests/parity/rust_runtime_configuration_models_5_0_6/Cargo.lock',
         'tests/generator/parity/business_ids_rust/PROVENANCE.json',
         'tests/generator/parity/business_ids_rust/lib/src/runtime.rs')),
}
MANIFEST_HASHES = {
    'rust_execution_shapes_5_0_6': '9a4b33d4344be147d1fcea65b62dff57617bd63620c9cf47f17540ba13e1c7c9',
    'rust_runtime_configuration_models_5_0_6': 'ad48c28ee4674b47fa48dca599326caa40eabbeefd45b9254acb4aae029ee560',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_inputs(record, record_name):
    expected_record_hash, expected_sources = SOURCE_RECORDS[record_name]
    record_path = ROOT / 'docs/parity' / record_name
    assert digest(record_path) == expected_record_hash, record_name + ': original record changed'
    assert same(record, json.loads(record_path.read_text())), record_name + ': loaded record changed'
    assert set(record['source_inputs']) == set(expected_sources), record_name + ': incomplete source inputs'
    assert record['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert record['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
    for relative, expected_hash in record['source_inputs'].items():
        assert digest(ROOT / relative) == expected_hash, relative
    project = ('rust_runtime_configuration_models_5_0_6' if record_name.startswith('generated_registry')
               else 'rust_execution_shapes_5_0_6')
    project_root = ROOT / 'tests/parity' / project
    assert digest(project_root / 'Cargo.toml') == MANIFEST_HASHES[project], project + ': manifest changed'
    assert registry_identities(project_root / 'Cargo.lock') <= registry_identities(
        ROOT / '.reference/teaql-rs/Cargo.lock'), project + ': registry dependency changed'
    if project == 'rust_execution_shapes_5_0_6':
        domain = ROOT / 'tests/generator/parity/dynamic_json_rust'
        provenance = json.loads((domain / 'PROVENANCE.json').read_text())
        assert provenance['hand_edited'] is False
        assert digest(ROOT / provenance['model']) == provenance['model_sha256']
        for relative, expected_hash in provenance['files'].items():
            assert digest(domain / relative) == expected_hash, relative


def load_original_record(record_name):
    record = json.loads((ROOT / 'docs/parity' / record_name).read_text())
    verify_inputs(record, record_name)
    return record


def run(executable, mode, environment):
    with tempfile.TemporaryDirectory(prefix='teaql_configuration_kernel_') as directory:
        result = subprocess.run([str(executable.resolve()), mode], cwd=directory,
                                env=environment, capture_output=True, check=True)
        paths = sorted(str(path.relative_to(directory))
                       for path in Path(directory).rglob('*'))
        return json.loads(result.stdout), paths


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust-config', type=Path)
    parser.add_argument('--rust-nul', type=Path)
    args = parser.parse_args()
    config = load_original_record('runtime_configuration_original_observations_5_0_6.json')
    nul = load_original_record('runtime_configuration_nul_original_observations_5_0_6.json')
    domain = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((domain / 'PROVENANCE.json').read_text())
    assert provenance['hand_edited'] is False
    for relative, digest in provenance['files'].items():
        assert hashlib.sha256((domain / relative).read_bytes()).hexdigest() == digest, relative
    assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
    count = 0
    for case in config['cases']:
        if case['mode'] != 'config':
            continue
        environment = dict(os.environb)
        environment.pop(b'CPP_DYNAMIC_JSON_CORE_DATABASE_URL', None)
        environment[b'TEAQL_LOG_ENDPOINT'] = b'off'
        if case['environment_hex'] is not None:
            environment[b'CPP_DYNAMIC_JSON_CORE_DATABASE_URL'] = bytes.fromhex(case['environment_hex'])
        expected = case['source_expected']
        if args.rust_config:
            actual, paths = run(args.rust_config, 'config', environment)
            assert same(expected, actual) and paths == [], case['name'] + ': original changed'
        actual, paths = run(args.cpp, 'config', environment)
        assert same(expected, actual) and paths == [], case['name'] + ': C++ mismatch'
        count += 1
    environment = dict(os.environb)
    environment[b'TEAQL_LOG_ENDPOINT'] = b'off'
    for case in nul['cases']:
        if args.rust_nul:
            actual, paths = run(args.rust_nul, case['name'], environment)
            assert same(case['source_expected'], actual) and paths == case['created_paths'], case['name'] + ': original changed'
        actual, paths = run(args.cpp, 'nul_' + case['name'], environment)
        assert same(case['source_expected'], actual) and paths == case['created_paths'], case['name'] + ': C++ mismatch'
        count += 1
    assert count == 9
    print('Verified9 actual-source configuration/NUL kernel outcomes; full runtime assembly remains separate')


if __name__ == '__main__':
    main()
