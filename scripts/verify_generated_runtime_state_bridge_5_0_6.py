#!/usr/bin/env python3
"""Qualify generated borrowed-root adapters without accepting whole generator parity."""
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
                  scope='Generated existing-root borrow, actual source models, Context ownership and retained Save integration', steps=[])

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
    project = ROOT / 'tests/parity/rust_generated_state_bridge_reference_5_0_6'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    record['exact_reference_registry_identities'] = True
    rust_env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml'], rust_env)
    run(['cmake', '--preset', args.profile])
    module = args.profile == 'modules_native'
    prefix = 'teaql_module_' if module else 'teaql_'
    stems = ['generated_runtime_state_bridge_probe', 'runtime_state_bridge_probe', 'ledger_composition_tests']
    names = ['generated_runtime_state_bridge_context_contracts', 'runtime_state_bridge_context_contracts',
             'generated_ledger_composition_contracts']
    if args.profile != 'sanitize':
        names += ['generated_runtime_state_bridge_rust_parity', 'runtime_state_bridge_rust_parity']
    if args.profile == 'native':
        stems += ['generated_api_tests', 'generated_dynamic_field_tests', 'generated_inheritance_graph_tests']
        names += ['generated_api_contracts', 'generated_dynamic_field_contracts', 'generated_inheritance_graph_contracts']
    if args.profile == 'sanitize':
        stems.append('generated_dynamic_field_tests')
        names.append('generated_dynamic_field_contracts')
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
    run(['python3', 'tests/parity/compare_generated_runtime_state_bridge_5_0_6.py', '--cpp',
         binary_root / (prefix + 'generated_runtime_state_bridge_probe'), '--rust',
         '/tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_generated_state_bridge_reference_506',
         '--record', comparison], environment)
    assert 'no work to do' in run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    paths = [ROOT / 'tests/parity/generated_runtime_state_bridge_probe.cpp',
             ROOT / 'tests/parity/compare_generated_runtime_state_bridge_5_0_6.py',
             ROOT / 'tests/parity/generated_runtime_state_bridge_5_0_6_cases.json.gz',
             project / 'src/main.rs', project / 'Cargo.toml', project / 'Cargo.lock', comparison,
             ROOT / 'teaql_runtime/include/teaql/runtime/detail/runtime_state_bridge.hpp',
             ROOT / 'teaql_runtime/include/teaql/runtime/detail/data_service/audited_auto_save.hpp',
             ROOT / 'tests/generated_typed_member_aliases/include/cpp_member_aliases/support.hpp',
             Path(__file__).resolve(), binary_root / (prefix + 'generated_runtime_state_bridge_probe')]
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    record.update(complete=True, actual_generated_source_observations=6)
    save()
    print('Qualified', args.profile, len(names), 'checks and6 complete actual generated source observations')


if __name__ == '__main__':
    main()
