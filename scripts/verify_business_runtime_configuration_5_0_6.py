#!/usr/bin/env python3
"""Record/replay original Business-ID runtime assembly and explicit C++ key adaptation."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same
from verify_constant_closure_5_0_6 import registry_identities

OUTPUT = ROOT / 'docs/parity/business_runtime_configuration_original_5_0_6.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_inputs():
    fixture = ROOT / 'tests/generator/parity/business_ids_rust'
    provenance = json.loads((fixture / 'PROVENANCE.json').read_text())
    assert provenance['hand_edited'] is False
    for path, expected in provenance['files'].items():
        assert digest(fixture / path) == expected, path
    assert digest(ROOT / provenance['model']) == provenance['model_sha256']
    lock = ROOT / 'tests/parity/rust_runtime_configuration_models_5_0_6/Cargo.lock'
    assert registry_identities(lock) <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    paths = ['tests/generator/parity/business_ids_rust/PROVENANCE.json',
             'tests/generator/parity/business_ids_rust/lib/src/runtime.rs',
             'tests/generator/business_ids.xml',
             'tests/parity/rust_runtime_configuration_models_5_0_6/Cargo.toml',
             'tests/parity/rust_runtime_configuration_models_5_0_6/Cargo.lock',
             'tests/parity/rust_runtime_configuration_models_5_0_6/src/main.rs']
    return {path: digest(ROOT / path) for path in paths}


def observe(executable, input_value, key_hex, key_version):
    environment = dict(os.environ)
    environment['TEAQL_LOG_ENDPOINT'] = 'off'
    environment['CPP_BUSINESS_IDS_CORE_DATABASE_URL'] = ':memory:'
    for name, value in [('TEAQL_BUSINESS_ID_KEY_V1_HEX', key_hex),
                        ('TEAQL_BUSINESS_ID_KEY_VERSION', key_version)]:
        environment.pop(name, None)
        if value is not None:
            environment[name] = value
    result = subprocess.run([str(executable.resolve())],
                            input=json.dumps(input_value) + '\n', env=environment,
                            text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'Failed observer ({result.returncode}): {result.stdout}\n{result.stderr}')
    return json.loads(result.stdout), result.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--output', type=Path, default=OUTPUT)
    args = parser.parse_args()
    inputs = source_inputs()
    runtime = json.loads((ROOT / 'upstream.lock.json').read_text())
    if args.record:
        if not args.rust or args.cpp or args.output.exists():
            parser.error('Recording requires --rust, forbids --cpp and never replaces evidence')
        cases = []
        for entry, rounds, version in itertools.product(('pool', 'config', 'env'), (1, 2, 3), (1, 2, 4294967295)):
            input_value = dict(entry=entry, rounds=rounds, key_version=version)
            expected, stderr = observe(args.rust, input_value, bytes(range(32)).hex(), str(version))
            cases.append(dict(input=input_value, source_expected=expected, stderr=stderr))
        errors = []
        for name, key_hex, version in [('missing_key', None, None),
                                       ('short_key', '00', '1'),
                                       ('invalid_hex', 'z' * 64, '1'),
                                       ('zero_version', bytes(range(32)).hex(), '0'),
                                       ('invalid_version', bytes(range(32)).hex(), 'bad'),
                                       ('overflow_version', bytes(range(32)).hex(), '4294967296')]:
            for entry in ('pool', 'config', 'env'):
                value = dict(entry=entry, rounds=1, key_version=1)
                expected, stderr = observe(args.rust, value, key_hex, version)
                errors.append(dict(name=name, input=value, key_hex=key_hex,
                                   key_version=version, source_expected=expected, stderr=stderr))
        args.output.write_text(json.dumps(dict(
            schema=1, runtime_commit=runtime['commit'],
            generator_commit='45e1572c4956ca963410d8ea436ead3c8857087a',
            source_inputs=inputs, cases=cases, source_environment_errors=errors,
            adaptation='C++ installs its application-owned typed key through UserContext. '
                       'The original environment parsing failures are retained as source-only '
                       'observations; they are never normalized into successful C++ startup.'), indent=2) + '\n')
        print(f'Recorded {len(cases)} original runtime/persistence scenarios and {len(errors)} original environment failures')
        return
    record = json.loads(args.output.read_text())
    assert record['runtime_commit'] == runtime['commit']
    assert record['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
    assert record['source_inputs'] == inputs
    assert len(record['cases']) == 27 and len(record['source_environment_errors']) == 18
    for case in record['cases']:
        value = case['input']
        for executable, label in [(args.rust, 'original'), (args.cpp, 'C++')]:
            if executable:
                observed, _ = observe(executable, value, bytes(range(32)).hex(), str(value['key_version']))
                assert same(observed, case['source_expected']), (label, value, observed, case['source_expected'])
    if args.rust:
        for case in record['source_environment_errors']:
            observed, _ = observe(args.rust, case['input'], case['key_hex'], case['key_version'])
            assert same(observed, case['source_expected']), case['name']
    if not args.rust and not args.cpp:
        parser.error('Execution replay requires --rust or --cpp')
    print('Verified 27 complete runtime/persistence scenarios (54 Context rounds); '
          '18 original environment failures remain separate from explicit C++ key installation')


if __name__ == '__main__':
    main()
