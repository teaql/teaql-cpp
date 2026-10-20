#!/usr/bin/env python3
"""Qualify complete error.rs value/trait/ownership contracts on both frontends.

Observers call the unchanged original runtime and canonical C++ implementation.
The historical check/Debug families overlap; counts are regression observations,
not distinct error variants. Commit execution and safe publication stay separate.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'tests/parity/rust_runtime_error_reference_5_0_6'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--rust-target', type=Path, default=Path('/tmp/teaql_cpp_state_baselines_506'))
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt; retain earlier results')
    record = {'complete': False, 'full_goal_accepted': False, 'steps': [], 'matrices': [],
              'scope': 'Complete runtime error value/trait source, native64 and imports; '
                       'not actual after-commit delivery or safe-log publication.'}
    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target.resolve()),
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
        print('exit', result.returncode, ' '.join(command[:6]), flush=True)
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        text = run([Path(binary).resolve()], ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                                    for value in inputs))
        if not text.endswith('\n'):
            raise RuntimeError('Missing final observation delimiter')
        values = [json.loads(line) for line in text.split('\n')[:-1]]
        if len(values) != len(inputs):
            raise RuntimeError('Incomplete observations')
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    reference = ROOT / '.reference/teaql-rs'
    if not registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock'):
        raise RuntimeError('Original dependency registry versions/checksums differ')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--bins'])
    units = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--lib'])
    refusals = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--doc'])
    if '3 passed; 0 failed' not in units or '4 passed; 0 failed' not in refusals:
        raise RuntimeError('Original generic trait contract test selection differs')
    record.update(original_trait_tests=3, original_compile_refusals=4)
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    record['baseline'] = baseline
    matrices = []
    paths = [reference / 'teaql-runtime/src/error.rs', reference / 'Cargo.lock',
             PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock', PROJECT / 'main.rs', PROJECT / 'lib.rs',
             ROOT / 'tests/parity/rust_values/src/bin/runtime_check_oracle.rs',
             ROOT / 'tests/parity/rust_values/src/fixture_value.rs', Path(__file__).resolve()]
    for corpus_name, oracle, probe in [
            ('runtime_error_extended_5_0_6_cases.json.gz', 'runtime_error_source_506', 'runtime_error_source'),
            ('runtime_check_baseline_5_0_6_cases.json.gz', 'runtime_check_oracle', 'runtime_check'),
            ('runtime_error_debug_baseline_5_0_6_cases.json.gz', 'runtime_check_oracle', 'runtime_check')]:
        corpus = ROOT / 'tests/parity' / corpus_name
        data = json.loads(gzip.decompress(corpus.read_bytes()))
        if data['baseline'] != baseline or data.get('pointer_bits', 64) != 64:
            raise RuntimeError('Error comparison corpus baseline/width differs')
        inputs = [case['input'] for case in data['cases']]
        expected = [case['expected'] for case in data['cases']]
        original_binary = args.rust_target / 'debug' / oracle
        original = observe(original_binary, inputs)
        if not same(original, expected):
            raise RuntimeError('Immutable observations differ from actual source: ' + corpus_name)
        matrices.append((corpus_name, probe, inputs, expected))
        record['matrices'].append({'name': corpus_name, 'observations': len(inputs),
                                   'original_outputs': original, 'profiles': {}})
        paths.extend([corpus, original_binary])
    contracts = ['runtime_check_error_contracts', 'error_generic_context_contracts',
                 'commit_audit_contracts', 'runtime_check_rust_parity',
                 'runtime_error_debug_rust_parity', 'runtime_error_source_rust_parity']
    stems = ['runtime_check_tests', 'error_generic_tests', 'commit_audit_tests',
             'runtime_check_probe', 'runtime_error_source_probe']
    for profile, build_dir, prefix in [
            ('native', ROOT / 'build_native_current', 'teaql_'),
            ('modules_native', ROOT / 'build_modules_native/modules', 'teaql_module_')]:
        targets = [prefix + stem for stem in stems]
        tests = [('module_' if profile == 'modules_native' else '') + name for name in contracts]
        if profile == 'native':
            targets.append('teaql_runtime_completion_tests')
            tests.append('runtime_generic_completion_contracts')
        run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
        xml = ET.parse(junit).getroot()
        cases = xml.findall('.//testcase')
        if len(cases) != len(tests) or any(case.find(tag) is not None
                                        for case in cases for tag in ('failure', 'error', 'skipped')):
            raise RuntimeError('Installed error contract selection did not pass completely')
        record.setdefault('profiles', {})[profile] = {'passed': len(tests), 'junit': junit.read_text()}
        for entry, (name, probe, inputs, expected) in zip(record['matrices'], matrices):
            binary = build_dir / (prefix + probe + '_probe')
            if probe == 'runtime_check' and run([binary, '--pointer-bits']).strip() != '64':
                raise RuntimeError('Current historical error corpus is native64-only')
            actual = observe(binary, inputs)
            if not same(actual, expected):
                raise RuntimeError('Current error observation differs: ' + profile + '/' + name)
            entry['profiles'][profile] = {'matching': True, 'complete_outputs': actual}
            paths.append(binary)
        if 'no work to do' not in run(['cmake', '--build', '--preset', profile,
                                      '--parallel', '2', '--target', *targets]):
            raise RuntimeError('Installed selected consumer build is not stable')
    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
    paths.extend(ROOT / name for name in (
        'teaql_runtime/include/teaql/runtime/error.hpp', 'teaql_runtime/include/teaql/runtime/error_debug.hpp',
        'teaql_runtime/include/teaql/runtime/context_error.hpp', 'teaql_runtime/src/error.cpp',
        'teaql_runtime/tests/runtime_check_tests.cpp', 'teaql_runtime/tests/error_generic_tests.cpp',
        'teaql_runtime/tests/commit_audit_tests.cpp', 'teaql_runtime/tests/runtime_completion_tests.cpp',
        'tests/parity/runtime_check_probe.cpp', 'tests/parity/runtime_error_source_probe.cpp',
        'tests/parity/debug_modes.hpp', 'tests/support/move_only_error_fixture.hpp',
        'tests/parity/compare_runtime_error_source_5_0_6.py', 'modules/contracts.json', 'CMakePresets.json'))
    record.update(complete=True, observations_per_frontend=sum(len(values) for _, _, values, _ in matrices),
                  normalization='none; native64-only historical location guard retained',
                  hashes={str(path): digest(path) for path in paths},
                  original_runtime_unmodified=True, production_cpp_unmodified=True)
    save()
    print('Complete error value/trait source gates pass; broader callers and P0-P7 remain open.')


if __name__ == '__main__':
    main()
