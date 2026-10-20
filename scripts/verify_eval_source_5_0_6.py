#!/usr/bin/env python3
"""Qualify complete eval.rs responsibilities, including runtime-only Indexed state."""
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
    record = dict(scope='Complete eval.rs six-variant availability and EvalResult surface; not full G13/P7',
                  complete=False, steps=[], profile=args.profile)

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
    project = ROOT / 'tests/parity/rust_core_primitives_reference_5_0_6'
    original = ROOT / '.reference/teaql-rs'
    reference_identities = registry_identities(original / 'Cargo.lock')
    assert registry_identities(project / 'Cargo.lock') <= reference_identities
    record['reference_registry'] = dict(exact_versions_checksums=True,
                                      observer_entries=len(registry_identities(project / 'Cargo.lock')))
    rust_environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
         '--bin', 'eval_indexed_reference_5_0_6', '--bin', 'eval_reference_5_0_6'], rust_environment)
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                  original / 'teaql-core/Cargo.toml', '--lib', 'eval::tests'], rust_environment)
    assert '3 passed; 0 failed; 0 ignored;' in output
    run(['cmake', '--preset', args.profile])
    module = args.profile == 'modules_native'
    prefix = 'teaql_module_' if module else 'teaql_'
    stems = ['eval_indexed_probe', 'eval_tests', 'eval_probe', 'loaded_snapshot_tests']
    names = ['eval_indexed_context_contracts', 'eval_context_contracts', 'loaded_snapshot_contracts']
    if args.profile != 'sanitize':
        stems.append('initialized_snapshot_cache_probe')
        names += ['eval_indexed_rust_parity', 'eval_rust_parity', 'initialized_snapshot_cache_rust_parity']
    else:
        stems.append('context_reference_tests')
        names.append('context_reference_contracts')
    targets = [prefix + stem for stem in stems]
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
    run(['python3', 'tests/parity/compare_eval_indexed_5_0_6.py', '--cpp',
         binary_root / (prefix + 'eval_indexed_probe'), '--rust',
         '/tmp/teaql_cpp_core_baselines_506/debug/eval_indexed_reference_5_0_6', '--record', comparison], environment)
    run(['python3', 'tests/parity/compare_eval_5_0_6.py', '--cpp', binary_root / (prefix + 'eval_probe'),
         '--rust', '/tmp/teaql_cpp_core_baselines_506/debug/eval_reference_5_0_6'], environment)
    assert 'no work to do' in run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    paths = [ROOT / 'teaql_core/include/teaql/core/eval.hpp', ROOT / 'teaql_core/src/row.cpp',
             ROOT / 'teaql_core/tests/eval_tests.cpp', ROOT / 'tests/parity/eval_indexed_probe.cpp',
             ROOT / 'tests/parity/compare_eval_indexed_5_0_6.py',
             ROOT / 'tests/parity/eval_indexed_5_0_6_cases.json.gz',
             project / 'src/bin/eval_indexed_reference_5_0_6.rs',
             project / 'src/bin/eval_reference_5_0_6.rs', project / 'Cargo.toml', project / 'Cargo.lock',
             original / 'teaql-core/src/eval.rs', Path(__file__).resolve(), comparison,
             binary_root / (prefix + 'eval_indexed_probe'), binary_root / (prefix + 'eval_probe')]
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    record.update(complete=True, original_unit_tests=3, old_eval_observations=1136,
                  state_scenarios=18, mutation_steps=180)
    save()
    print('Qualified', args.profile, len(names), 'checks, 1136 retained observations and18/180 new state scenarios/steps')


if __name__ == '__main__':
    main()
