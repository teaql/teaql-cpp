#!/usr/bin/env python3
"""Qualify metadata-aware runtime lookup before changing generated adapters."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native', 'sanitize'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh record; preserve previous evidence')
    module = args.profile == 'modules_native'
    record = dict(scope='Runtime lookup only; generated adapters and whole G15 remain pending',
                  complete=False, profile=args.profile, steps=[], hashes={})

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(a) for a in command], cwd=ROOT, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(a) for a in command],
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(a) for a in command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    record['runtime_commit'] = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    run(['cmake', '--preset', args.profile])
    names = ['runtime_entity_expression_contracts', 'context_reference_contracts']
    stems = ['runtime_expression_tests', 'context_reference_tests']
    if args.profile != 'sanitize':
        names.extend(['field_layout_contracts', 'generated_dynamic_field_contracts'])
        stems.extend(['field_layout_tests', 'generated_dynamic_field_tests'])
    if args.profile == 'native':
        names.append('generated_api_contracts')
        stems.append('generated_api_tests')
    prefix = 'teaql_module_' if module else 'teaql_'
    targets = [prefix + stem for stem in stems]
    run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    pattern = '^(' + '|'.join(('module_' if module else '') + name for name in names) + ')$'
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off')
    if args.profile == 'sanitize':
        environment.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
        record['sanitizer_environment'] = {key: environment[key] for key in ['ASAN_OPTIONS', 'UBSAN_OPTIONS']}
    output = run(['ctest', '--preset', args.profile, '--output-on-failure', '-R', pattern], environment)
    if not re.search(rf'100% tests passed out of {len(names)}\b', output):
        raise RuntimeError('Incomplete selected test result')
    record['ctest_passed'] = len(names)
    rust_environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
         'tests/parity/rust_field_layout/Cargo.toml', '--bin', 'typed_member_alias_5_0_6'], rust_environment)
    rust_binary = Path(rust_environment['CARGO_TARGET_DIR']) / 'debug/typed_member_alias_5_0_6'
    build_dir = ROOT / {'native': 'build_native_current', 'modules_native': 'build_modules_native',
                        'sanitize': 'build_sanitize_current'}[args.profile]
    cpp_binary = build_dir / ('modules' if module else '') / (prefix + 'runtime_expression_tests')
    original = run([rust_binary])
    actual = run([cpp_binary, '--alias-probe'], environment)
    if original != actual or len(original.splitlines()) != 130:
        raise RuntimeError('Actual original/C++ alias observations differ or are incomplete')
    record['actual_source_comparisons'] = 130
    record['original'] = original.splitlines()
    record['cpp'] = actual.splitlines()
    output = run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    if 'no work to do' not in output:
        raise RuntimeError('Implementation changed during qualification')
    if module:
        run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
    for path in [Path(__file__).resolve(), rust_binary, cpp_binary,
                 ROOT / 'teaql_runtime/include/teaql/runtime/entity_expression.hpp',
                 ROOT / 'teaql_runtime/tests/entity_expression_tests.cpp',
                 ROOT / 'tests/parity/rust_field_layout/src/bin/typed_member_alias_5_0_6.rs',
                 ROOT / 'tests/parity/rust_field_layout/Cargo.lock',
                 ROOT / '.reference/teaql-rs/teaql-core/tests/shared_load_state.rs',
                 ROOT / '.reference/teaql-rs/teaql-macros/src/derive_impl.rs',
                 ROOT / 'modules/contracts.json', ROOT / 'CMakePresets.json']:
        record['hashes'][str(path)] = digest(path)
    before_path = ROOT / 'docs/parity/typed_member_alias_runtime_before_5_0_6.json.gz'
    before = json.loads(gzip.decompress(before_path.read_bytes()))
    generator_path = next(path for path in before['files'] if path.endswith('CppCoreGenerator.java'))
    if digest(Path(generator_path)) != before['files'][generator_path]['sha256']:
        raise RuntimeError('Generator changed before runtime qualification')
    record['unchanged_generator'] = dict(path=generator_path, sha256=digest(Path(generator_path)))
    record['before_record'] = dict(path=str(before_path.relative_to(ROOT)), sha256=digest(before_path))
    record['complete'] = True
    save()
    print('Qualified runtime130 actual-source outcomes and', len(names), 'contracts;',
          'generated adapter remains pending', flush=True)


if __name__ == '__main__':
    main()
