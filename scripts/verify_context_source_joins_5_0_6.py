#!/usr/bin/env python3
"""Recheck UserContext's delegated contracts without promoting child sources."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
STEMS = (
    'context_event_tests', 'context_event_probe', 'telemetry_tests', 'telemetry_probe',
    'telemetry_model_scope_probe', 'i18n_tests', 'i18n_probe', 'i18n_debug_scope_probe',
    'schema_dynamic_tests', 'schema_dynamic_source_probe', 'schema_runtime_tests',
    'schema_runtime_probe', 'commit_audit_tests', 'commit_audit_source_probe',
    'round_trip_reference_tests', 'round_trip_reference_probe', 'entity_reference_tests',
    'entity_reference_probe', 'document_service_async_probe', 'document_providers_probe',
    'runtime_state_bridge_probe', 'generated_runtime_state_bridge_probe',
)
TESTS = (
    'context_event_contracts', 'telemetry_contracts', 'i18n_context_contracts',
    'schema_dynamic_context_contracts', 'schema_runtime_contracts', 'commit_audit_contracts',
    'round_trip_reference_context_contracts', 'entity_reference_context_contracts',
    'runtime_state_bridge_context_contracts', 'generated_runtime_state_bridge_context_contracts',
    'context_event_rust_parity', 'telemetry_rust_parity', 'telemetry_model_scope_rust_parity',
    'i18n_rust_parity', 'i18n_debug_scope_rust_parity', 'schema_runtime_rust_parity',
    'schema_dynamic_source_rust_parity', 'round_trip_reference_rust_parity',
    'entity_reference_rust_parity', 'runtime_state_bridge_rust_parity',
    'generated_runtime_state_bridge_rust_parity', 'document_service_async_rust_parity',
    'document_providers_rust_parity',
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh evidence path')
    record = dict(complete=False, full_goal_accepted=False, steps=[], profiles={},
                  scope='Current delegated Context contracts; child source and capability exits remain independent.')
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(),
                                             compresslevel=1, mtime=0))

    def run(command):
        result = subprocess.run([str(part) for part in command], cwd=ROOT, env=env,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(part) for part in command],
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'tests/parity/build_context_event_oracle_5_0_6.py'])
    provenance = ROOT/'build_rust_context_event_5_0_6/provenance.json'
    record['event_source_provenance'] = json.loads(provenance.read_text())
    assert record['event_source_provenance']['build_status'] == 'completed'
    assert record['event_source_provenance']['original_registry_versions_preserved']
    original = Path('/tmp/teaql_cpp_context_event_rust_506/debug/teaql_context_event_506')
    assert digest(original) == record['event_source_provenance']['binary_sha256']
    for profile in ('native', 'modules_native'):
        modules = profile == 'modules_native'
        prefix = 'teaql_module_' if modules else 'teaql_'
        build = ROOT/('build_modules_native' if modules else 'build_native_current')
        binaries = build/'modules' if modules else build
        targets = [prefix+stem for stem in STEMS]
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        run(['python3', 'tests/parity/compare_context_event.py', '--rust', original,
             '--cpp', binaries/(prefix+'context_event_probe')])
        run(['python3', 'tests/parity/compare_commit_audit_source_5_0_6.py',
             '--cpp', binaries/(prefix+'commit_audit_source_probe')])
        selected = [('module_' if modules else '')+name for name in TESTS]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest', '--test-dir', build, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^('+'|'.join(selected)+')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks) == len(selected)
        assert {row.attrib['name'] for row in checks} == set(selected)
        assert not any(row.find(tag) is not None for row in checks for tag in ('failure','error','skipped'))
        record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text(),
                                          binaries={target:digest(binaries/target) for target in targets})
        assert 'no work to do' in run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        save()
        print(profile, len(checks), 'delegation checks; original Context event and retained commit audit match', flush=True)
    record.update(complete=True, controller_sha256=digest(Path(__file__).resolve()))
    save()


if __name__ == '__main__':
    main()
