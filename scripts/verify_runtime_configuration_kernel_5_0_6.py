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


def verify_inputs(record):
    assert record['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert record['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
    for relative, digest in record['source_inputs'].items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest, relative


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
    config = json.loads((ROOT / 'docs/parity/runtime_configuration_original_observations_5_0_6.json').read_text())
    nul = json.loads((ROOT / 'docs/parity/runtime_configuration_nul_original_observations_5_0_6.json').read_text())
    verify_inputs(config)
    verify_inputs(nul)
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
