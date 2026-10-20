#!/usr/bin/env python3
"""Replay complete SQLite provider root evidence on the locked Rust and C++ frontends."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

from verify_constant_closure_5_0_6 import registry_identities
from run_native_integration_epoch_5_0_6 import capture_inputs

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_sqlite_provider_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('sqlite_schema', 'sqlite_schema_oracle', 'sqlite_schema_probe', 735, ()),
    ('sqlite_decoding', 'sqlite_decoding_oracle', 'sqlite_decoding_checked_probe', 6029, ()),
    ('sqlite_bindings', 'sqlite_bindings_oracle', 'sqlite_bindings_probe', 1084, ()),
    ('sqlite_error_model', 'sqlite_error_model_oracle', 'sqlite_error_model_probe', 563, ()),
    ('sqlite_native_diagnostics', 'sqlite_native_diagnostics_oracle', 'sqlite_native_diagnostics_probe', 5810, ()),
    ('sqlite_simple_error', 'sqlite_simple_error_oracle', 'sqlite_simple_error_probe', 62, ()),
    ('sqlite_from_sql_error', 'sqlite_from_sql_error_oracle', 'sqlite_from_sql_error_probe', 36, ()),
    ('sqlite_from_sql_raw_debug', 'sqlite_from_sql_error_oracle', 'sqlite_from_sql_error_probe', 36, ('--raw-debug',)),
    ('sqlite_transport', 'sqlite_provider_transport_source_506', 'sqlite_transport_probe', 798, ()),
    ('sqlite_id', 'sqlite_provider_transport_source_506', 'sqlite_transport_probe', 261, ()),
    ('sqlite_business_id', 'sqlite_provider_transport_source_506', 'sqlite_transport_probe', 198, ()),
)
UNITS = dict(
    original_provider_storage_tests='original_provider_storage_contracts',
    original_provider_schema_tests='original_provider_schema_contracts',
    original_provider_query_tests='original_provider_query_contracts',
    original_provider_transaction_tests='original_provider_transaction_contracts',
    provider_sqlite_model_tests='provider_sqlite_model_contracts',
    provider_sqlite_unicode_tests='provider_sqlite_unicode_contracts',
    provider_sqlite_business_id_tests='provider_sqlite_business_id_contracts',
    provider_sqlite_driver_error_tests='provider_sqlite_driver_error_contracts',
    provider_sqlite_error_tests='provider_sqlite_error_contracts',
    provider_sqlite_simple_error_tests='provider_sqlite_simple_error_contracts',
    provider_sqlite_from_sql_error_tests='provider_sqlite_from_sql_error_contracts',
    provider_sqlite_boxed_error_tests='provider_sqlite_boxed_error_contracts',
    sqlite_tests='sqlite_contracts',
    provider_sqlite_tests='provider_sqlite_dialect_contracts',
    provider_sqlite_bindings_tests='provider_sqlite_binding_contracts',
    provider_sqlite_decoding_tests='provider_sqlite_decoding_contracts',
    provider_sqlite_transport_tests='provider_sqlite_transport_contracts',
    provider_sqlite_schema_tests='provider_sqlite_schema_contracts',
    provider_sqlite_id_tests='provider_sqlite_id_contracts',
    native_transaction_stream_tests='provider_sqlite_transaction_stream_contracts',
    dynamic_storage_tests='dynamic_storage_contracts',
    dynamic_query_tests='dynamic_query_contracts',
)
CONTROLS = (
    'sqlite_chunk_capacity_rust_parity', 'sqlite_chunk_capacity_contracts',
    'sqlite_lease_binding_rust_parity', 'sqlite_lease_binding_guard_rust_parity',
    'sqlite_async_lease_rust_parity',
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native'),
                        default=['native', 'modules_native'])
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path; historical evidence is immutable')
    record = dict(complete=False, whole_provider_accepted=False, full_goal_accepted=False,
                  steps=[], matrices=[], profiles={}, normalization='none',
                  width_scope='Native 64-bit raw driver Debug; checked Debug timestamp profile')
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
            stdout=result.stdout if payload is None else None,
            stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(), stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs, arguments=()):
        output = run([binary, *arguments], ''.join(json.dumps(row) + '\n' for row in inputs))
        rows = [json.loads(line) for line in output.split('\n') if line]
        assert len(rows) == len(inputs), (binary, len(rows), len(inputs))
        return rows

    run(['python3', 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    reference = ROOT / '.reference/teaql-rs'
    identities = registry_identities(PROJECT / 'Cargo.lock')
    assert identities <= registry_identities(reference / 'Cargo.lock')
    record.update(baseline=baseline, observer_manifest_sha256=digest(PROJECT / 'Cargo.toml'),
                  observer_lock_sha256=digest(PROJECT / 'Cargo.lock'),
                  verifier_sha256=digest(Path(__file__)), exact_registry_identities=len(identities),
                  execution_inputs=capture_inputs(),
                  reference_source={str(p.relative_to(ROOT)): digest(p)
                      for p in (reference / 'teaql-provider-sqlite/src').rglob('*.rs')})
    save()
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bins', '-j', '2'])
    root_test = ['cargo', 'test', '--offline', '--locked', '--manifest-path',
                 reference / 'teaql-provider-sqlite/Cargo.toml', '--lib', '-j', '2']
    registry = run(root_test + ['--', '--list'])
    names = re.findall(r'^([^ ]+): test$', registry, re.M)
    wanted = [name for name in names if name.startswith('tests::')]
    assert len(wanted) == 42, wanted
    skips = []
    for other in sorted({name.split('::', 1)[0] for name in names if not name.startswith('tests::')}):
        skips += ['--skip', other + '::']
    assert '42 passed; 0 failed' in run(root_test + ['tests::', '--', '--nocapture', *skips])
    record['original_root_tests'] = wanted
    matrices = []
    for name, rust, probe, count, arguments in MATRICES:
        path = PARITY / (name + '_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(path.read_bytes()))
        assert corpus['baseline'] == baseline
        assert len(corpus['cases']) == count
        driver = corpus.get('original_driver', corpus.get('driver'))
        if driver:
            assert digest(ROOT / driver) == corpus.get('original_driver_sha256', corpus.get('driver_sha256'))
        inputs = [case['input'] for case in corpus['cases']]
        expected = [case['expected'] for case in corpus['cases']]
        binary = Path(env['CARGO_TARGET_DIR']) / 'debug' / rust
        actual = observe(binary, inputs, arguments)
        assert same(actual, expected), ('original', name)
        matrices.append((name, probe, inputs, expected, arguments))
        record['matrices'].append(dict(name=name, cases=count, corpus_sha256=digest(path),
            original_binary_sha256=digest(binary), profiles={}))
        save()

    controls_path = ROOT / 'docs/parity/sqlite_baselines_5_0_6.json'
    controls = json.loads(controls_path.read_text())['timestamp_short_circuit_controls']
    assert len(controls) == 64
    inputs = [row['input'] for row in controls]
    expected = [row['expected'] for row in controls]
    binary = Path(env['CARGO_TARGET_DIR']) / 'debug/sqlite_decoding_oracle'
    assert same(observe(binary, inputs), expected), 'original timestamp controls'
    matrices.append(('timestamp_short_circuit', 'sqlite_decoding_checked_probe', inputs, expected, ()))
    record['matrices'].append(dict(name='timestamp_short_circuit', cases=len(inputs),
        corpus_sha256=digest(controls_path), original_binary_sha256=digest(binary), profiles={}))

    for profile in args.profiles:
        imported = profile == 'modules_native'
        build = 'build_modules_native' if imported else 'build_native_current'
        prefix = 'teaql_module_' if imported else 'teaql_'
        stems = list(dict.fromkeys([row[1] for row in matrices] + list(UNITS) +
                     ['sqlite_chunk_capacity_probe', 'sqlite_lease_binding_probe', 'sqlite_async_lease_probe']))
        targets = [prefix + stem for stem in stems]
        run(['cmake', '--preset', profile])
        build_command = ['cmake', '--build', build, '--target', *targets, '--parallel', '4']
        run(build_command)
        for index, (name, probe, inputs, expected, arguments) in enumerate(matrices):
            binary = ROOT / build / ('modules' if imported else '') / (prefix + probe)
            if name == 'sqlite_decoding':
                assert run([binary, '--profile']).strip() == 'checked'
            actual = observe(binary, inputs, arguments)
            gaps = [dict(index=i, input=inputs[i], original=a, actual=b)
                    for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
            record['matrices'][index]['profiles'][profile] = dict(binary_sha256=digest(binary), differences=gaps)
            save()
            assert not gaps, (profile, name, len(gaps))
        tests = [('module_' if imported else '') + name for name in (*UNITS.values(), *CONTROLS)]
        junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
        assert not junit.exists()
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert {check.attrib['name'] for check in checks} == set(tests) and len(checks) == len(tests)
        assert not any(check.find(tag) is not None for check in checks for tag in ('failure', 'error', 'skipped'))
        stable = run(build_command)
        if 'ninja: no work to do.' not in stable:
            stable = run(build_command)
        assert 'ninja: no work to do.' in stable
        record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text(), no_work_rebuild=True)
        save()
        print(profile, '15676 exact source outcomes;', len(checks), 'checks; no work', flush=True)

    after = capture_inputs()
    assert after == record['execution_inputs'], 'Execution inputs changed during the qualification epoch'
    assert all(digest(ROOT / name) == value for name, value in record['reference_source'].items())
    record.update(complete=True, original_tests_passed=42, source_outcomes_per_frontend=15676,
                  unchanged_execution_inputs=True)
    save()


if __name__ == '__main__':
    main()
