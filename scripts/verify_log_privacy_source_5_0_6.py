#!/usr/bin/env python3
"""Qualify complete SQLite-scope SQL provenance and privacy/masking sources.

Run byte-checked original private tests, unchanged public/private observers and
canonical native/imported consumers. No query, projection or masking algorithm
is implemented here. Publication/custom sinks and whole goal remain separate.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'tests/parity/rust_log_privacy_reference_5_0_6'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same

MATRICES = (
    ('sql_log', 'sql_log_source_sql_log', 'sql_log', 1008),
    ('sql_projection_scope', 'sql_log_source_sql_log', 'sql_projection_scope', 8),
    ('sql_model_api', 'sql_log_source_sql_model_api', 'sql_model_api', 40),
    ('sql_log_query_source', 'sql_log_source_query', 'sql_log_query_source', 79),
    ('runtime_log_baseline', 'runtime_log_oracle', 'runtime_log', 425),
    ('audit_event_baseline', 'audit_event_oracle', 'audit_event', 1427),
    ('audit_event_projection', 'privacy_projection', 'audit_event_projection', 104),
)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--rust-target', type=Path, default=Path('/tmp/teaql_cpp_state_baselines_506'))
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt; retain earlier evidence')
    record = {'complete': False, 'steps': [], 'matrices': [], 'full_goal_accepted': False,
              'scope': 'Complete sql_log.rs/log_privacy.rs and SQLite-applicable masking tests; '
                       'native64/imports, not whole Context/governed execution/publication.'}
    environment = {key: value for key, value in os.environ.items() if not key.startswith('TEAQL_')}
    environment.update(CARGO_TARGET_DIR=str(args.rust_target.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, inputs=None):
        command = [str(part) for part in command]
        result = subprocess.run(command, cwd=ROOT, env=environment,
                                input=inputs, text=True, capture_output=True)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'stdout': result.stdout, 'stderr': result.stderr})
        save()
        print('exit', result.returncode, ' '.join(command[:7]), flush=True)
        result.check_returncode()
        return result.stdout

    def observe(binary, values):
        text = run([Path(binary).resolve()], ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                                    for value in values))
        if not text.endswith('\n'):
            raise RuntimeError('Missing final JSONL delimiter')
        outputs = [json.loads(line) for line in text.split('\n')[:-1]]
        if len(outputs) != len(values):
            raise RuntimeError('Incomplete source comparison output')
        return outputs

    run(['python3', 'scripts/verify_upstream.py'])
    reference = ROOT / '.reference/teaql-rs'
    if not registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock'):
        raise RuntimeError('Original observer dependencies differ from the exact reference lock')
    sql_tests = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2', '--manifest-path',
                     reference / 'teaql-data-service/Cargo.toml', '--lib', 'sql_log::query_intent_tests::'])
    if '2 passed; 0 failed' not in sql_tests:
        raise RuntimeError('Complete original query-intent test selection differs')
    record['original_sql_log_tests'] = 2
    with tempfile.TemporaryDirectory(prefix='teaql_log_privacy_source_506_') as temporary:
        view = ReferenceRuntimeView(ROOT, Path(temporary) / 'original_runtime', {}, run)
        tests = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2', '--manifest-path',
                     view.directory / 'Cargo.toml', '--lib', 'log_privacy::', '--',
                     '--nocapture', '--test-threads=1'])
        if ('40 passed; 0 failed; 1 ignored' not in tests
                or 'file_log_gate_runs_in_isolated_processes ... ok' not in tests):
            raise RuntimeError('Complete original privacy/masking tests or isolated parent did not pass')
        record.update(original_privacy_masking_tests=40,
                      original_ignored_test='file_log_child: exercised by the passing parent in three processes',
                      cpp_backend_exclusion='masked_postgres_array_preserves_shape_and_masks_each_element',
                      original_runtime_view=view.provenance())
        save()
    run(['cargo', 'build', '--offline', '--locked', '--jobs', '2', '--manifest-path',
         PROJECT / 'Cargo.toml', '--bins'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    record['baseline'] = baseline
    matrices = []
    paths = [Path(__file__), ROOT / 'scripts/reference_runtime_view_5_0_6.py',
             PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock', PROJECT / 'query.rs', reference / 'Cargo.lock']
    for stem, oracle, probe, count in MATRICES:
        path = ROOT / 'tests/parity' / (stem + '_5_0_6_cases.json.gz')
        data = json.loads(gzip.decompress(path.read_bytes()))
        if data['baseline'] != baseline or len(data['cases']) != count:
            raise RuntimeError('Immutable source corpus baseline/count differs: ' + stem)
        inputs = [case['input'] for case in data['cases']]
        expected = [case['expected'] for case in data['cases']]
        original = observe(args.rust_target / 'debug' / oracle, inputs)
        if not same(original, expected):
            raise RuntimeError('Retained expectations differ from actual source: ' + stem)
        record['matrices'].append({'name': stem, 'observations': count, 'original': original, 'profiles': {}})
        matrices.append((stem, probe, inputs, expected))
        paths.extend([path, args.rust_target / 'debug' / oracle])
    tests = ['data_service_contracts', 'executor_spi_contracts', 'sql_projection_scope_contracts',
             'sql_log_rust_parity', 'sql_projection_scope_rust_parity', 'sql_model_api_rust_parity',
             'sql_log_query_source_rust_parity', 'runtime_log_contracts', 'audit_event_contracts',
             'audit_event_context_reentrant_close', 'audit_event_owner_reentrant_close',
             'audit_event_payload_reentrant_close', 'audit_event_result_reentrant_close',
             'original_runtime_log_privacy_contracts', 'original_runtime_masking_contracts',
             'original_runtime_file_log_privacy_contracts', 'runtime_log_rust_parity',
             'audit_event_rust_parity', 'audit_event_projection_5_0_6_rust_parity']
    common = ['data_service_tests', 'executor_tests', 'sql_projection_scope_tests',
              'sql_log_probe', 'sql_projection_scope_probe', 'sql_model_api_probe',
              'sql_log_query_source_probe', 'runtime_log_tests', 'audit_event_tests',
              'runtime_log_probe', 'audit_event_probe', 'audit_event_projection_probe']
    for profile, build, prefix in [
            ('native', ROOT / 'build_native_current', 'teaql_'),
            ('modules_native', ROOT / 'build_modules_native/modules', 'teaql_module_')]:
        native = profile == 'native'
        targets = [prefix + stem for stem in common +
                   (['original_log_privacy_tests', 'original_masking_tests'] if native else
                    ['log_privacy', 'masking_contract_tests'])]
        run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
        selected = [name if native else 'module_' + name for name in tests]
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^(' + '|'.join(selected) + ')$'])
        cases = ET.parse(junit).getroot().findall('.//testcase')
        if len(cases) != len(selected) or any(case.find(tag) is not None
                                           for case in cases for tag in ('failure', 'error', 'skipped')):
            raise RuntimeError('Selected current frontend contracts did not all pass')
        record.setdefault('profiles', {})[profile] = {'passed': len(selected), 'junit': junit.read_text()}
        for entry, (stem, probe, inputs, expected) in zip(record['matrices'], matrices):
            binary = build / (prefix + probe + '_probe')
            actual = observe(binary, inputs)
            if not same(actual, expected):
                raise RuntimeError('Current frontend/source difference: ' + profile + '/' + stem)
            entry['profiles'][profile] = {'matching': True, 'complete_outputs': actual}
            paths.append(binary)
        if 'no work to do' not in run(['cmake', '--build', '--preset', profile,
                                      '--parallel', '2', '--target', *targets]):
            raise RuntimeError('Selected consumer build is not stable')
    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
    for name in ['teaql-data-service/src/sql_log.rs', 'teaql-runtime/src/log_privacy.rs',
                 'teaql-runtime/src/masking_contract_tests.rs']:
        paths.append(reference / name)
    paths.extend(ROOT / value for value in (
        'tests/parity/rust_field_layout/src/bin/sql_log_5_0_6.rs',
        'tests/parity/rust_field_layout/src/bin/sql_model_api_5_0_6.rs',
        'tests/parity/rust_field_layout/src/bin/audit_event_projection_5_0_6.rs',
        'tests/parity/rust_values/src/bin/runtime_log_oracle.rs',
        'tests/parity/rust_values/src/bin/audit_event_oracle.rs',
        'tests/parity/rust_values/src/formatter_fixture.rs',
        'tests/parity/teaql_runtime/log_privacy.cpp', 'tests/parity/teaql_runtime/masking_contract_tests.cpp',
        'tests/parity/teaql_runtime/file_log_privacy.py', 'tests/parity/teaql_runtime/data/masking_v1.tsv',
        'tests/parity/sql_log_query_source_probe.cpp', 'modules/contracts.json', 'CMakePresets.json'))
    record.update(complete=True, observations_per_frontend=3091,
                  hashes={str(path): digest(path) for path in paths},
                  normalization='No new normalization; unchanged observers retain their human UTC adapter',
                  production_cpp_changed=False, original_sources_changed=False)
    save()
    print('Complete SQL provenance/privacy/masking source gates pass; full capabilities remain open.')


if __name__ == '__main__':
    main()
