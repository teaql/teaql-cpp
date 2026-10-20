#!/usr/bin/env python3
"""Qualify runtime-first erased-state dispatch before generation emits adapters."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native', 'sanitize'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, complete_generator_goal=False, profile=args.profile,
                  scope='Runtime-first source carrier dispatch, alias and lifetime qualification; generator adapter/artifact gates separate', steps=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(x) for x in command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    project = ROOT / 'tests/parity/rust_document_services_reference_5_0_6'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    record['exact_reference_registry_identities'] = True
    rust_env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
         '--bin', 'runtime_state_bridge_5_0_6'], rust_env)
    run(['cmake', '--preset', args.profile])
    module = args.profile == 'modules_native'
    prefix = 'teaql_module_' if module else 'teaql_'
    stems = ['runtime_state_bridge_probe', 'audited_auto_save_tests', 'audited_auto_save_probe',
             'dynamic_graph_save_tests', 'context_reference_tests']
    names = ['runtime_state_bridge_context_contracts', 'audited_auto_save_context_contracts',
             'dynamic_graph_save_contracts', 'context_reference_contracts']
    if args.profile != 'sanitize':
        names += ['runtime_state_bridge_rust_parity', 'audited_auto_save_rust_parity']
    targets = [prefix + stem for stem in stems]
    if module:
        targets.append('teaql_module_contracts')
        names.append('import_contracts')
    elif args.profile == 'native':
        targets.append('teaql_generated_query_expression')
        names.append('example_generated_query_expression')
    run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    environment = dict(os.environ)
    if args.profile == 'sanitize':
        environment.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
    record['environment'] = {key: environment[key] for key in ['ASAN_OPTIONS', 'UBSAN_OPTIONS'] if key in environment}
    pattern = '^(' + '|'.join(('module_' if module else '') + name for name in names) + ')$'
    output = run(['ctest', '--preset', args.profile, '--output-on-failure', '-R', pattern], environment)
    assert re.search(rf'100% tests passed out of {len(names)}\b', output)
    record['passed'] = len(names)
    build = ROOT / {'native': 'build_native_current', 'modules_native': 'build_modules_native',
                    'sanitize': 'build_sanitize_current'}[args.profile]
    binary_root = build / 'modules' if module else build
    comparison = args.record.with_name(args.record.name.replace('.json.gz', '_source.json.gz'))
    run(['python3', 'tests/parity/compare_runtime_state_bridge_5_0_6.py', '--cpp',
         binary_root / (prefix + 'runtime_state_bridge_probe'), '--rust',
         '/tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/runtime_state_bridge_5_0_6', '--record', comparison], environment)
    run(['python3', 'tests/parity/compare_audited_auto_save_5_0_6.py', '--cpp',
         binary_root / (prefix + 'audited_auto_save_probe')], environment)
    assert 'no work to do' in run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    paths = [ROOT / 'teaql_runtime/include/teaql/runtime/detail/runtime_state_bridge.hpp',
             ROOT / 'teaql_runtime/include/teaql/runtime/detail/data_service/audited_auto_save.hpp',
             ROOT / 'teaql_runtime/src/lib.cppm', ROOT / 'modules/tests/teaql_runtime_lookup.cpp',
             ROOT / 'tests/parity/runtime_state_bridge_probe.cpp',
             ROOT / 'tests/parity/compare_runtime_state_bridge_5_0_6.py',
             ROOT / 'tests/parity/runtime_state_bridge_5_0_6_cases.json.gz',
             project / 'src/bin/runtime_state_bridge_5_0_6.rs', project / 'Cargo.toml', project / 'Cargo.lock',
             ROOT / '.reference/teaql-rs/teaql-runtime/src/entity_save.rs', comparison,
             Path(__file__).resolve(), binary_root / (prefix + 'runtime_state_bridge_probe')]
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    record.update(complete=True, actual_source_observations=24, retained_auto_save_observations=944,
                  generated_adapter_qualified=False)
    save()
    print('Qualified', args.profile, len(names), 'checks;24 direct and944 retained automatic-save observations')


if __name__ == '__main__':
    main()
