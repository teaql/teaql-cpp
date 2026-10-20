#!/usr/bin/env python3
"""Qualify original SQLite chunk bounds and native Context cleanup consumers."""
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
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_sqlite_chunk_capacity_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native', 'sanitize'),
                        default=['native'])
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path; retain all historical evidence')
    record = dict(complete=False, whole_provider_accepted=False, full_goal_accepted=False,
                  steps=[], profiles={})
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', ASAN_OPTIONS='detect_leaks=0',
               CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        output = run([binary], ''.join(json.dumps(row) + '\n' for row in inputs))
        rows = [json.loads(line) for line in output.split('\n') if line]
        assert len(rows) == len(inputs)
        return rows

    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(
        ROOT / '.reference/teaql-rs/Cargo.lock')
    record.update(manifest_sha256=digest(PROJECT / 'Cargo.toml'),
                  lock_sha256=digest(PROJECT / 'Cargo.lock'),
                  verifier_sha256=digest(Path(__file__)),
                  provider_sha256=digest(ROOT / 'teaql_provider_sqlite/src/provider.cpp'),
                  probe_source_sha256=digest(PARITY / 'sqlite_chunk_capacity_probe.cpp'))
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bins', '-j', '2'])
    original_test = ROOT / '.reference/teaql-rs/teaql-provider-sqlite/tests/stream_chunk_allocations.rs'
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                  ROOT / '.reference/teaql-rs/teaql-provider-sqlite/Cargo.toml',
                  '--test', 'stream_chunk_allocations', '-j', '2',
                  '--', '--nocapture', '--test-threads=1'])
    assert '3 passed; 0 failed' in output
    record.update(original_tests=3, original_test_sha256=digest(original_test),
                  counter_sha256=digest(ROOT / '.reference/teaql-rs/teaql-runtime/tests/support/allocation_counter.rs'))
    path = PARITY / 'sqlite_chunk_capacity_5_0_6_cases.json.gz'
    corpus = json.loads(gzip.decompress(path.read_bytes()))
    assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert digest(ROOT / corpus['original_driver']) == corpus['original_driver_sha256']
    inputs = [case['input'] for case in corpus['cases']]
    expected = [case['expected'] for case in corpus['cases']]
    assert len(inputs) == 858
    assert same(observe(Path(env['CARGO_TARGET_DIR']) / 'debug/sqlite_chunk_capacity_506', inputs), expected)
    record['corpus_sha256'] = digest(path)

    for profile in args.profiles:
        imported = profile == 'modules_native'
        prefix = 'teaql_module_' if imported else 'teaql_'
        case_prefix = 'module_' if imported else ''
        stems = ['sqlite_chunk_capacity_probe', 'sqlite_lease_binding_probe',
                 'native_transaction_stream_tests', 'dynamic_query_tests']
        names = ['sqlite_chunk_capacity_rust_parity', 'sqlite_chunk_capacity_contracts',
                 'sqlite_lease_binding_rust_parity', 'sqlite_lease_binding_guard_rust_parity',
                 'provider_sqlite_transaction_stream_contracts', 'dynamic_query_contracts']
        if profile == 'native':
            stems += ['original_provider_storage_tests']
            names += ['original_provider_storage_contracts']
        names = [case_prefix + name for name in names]
        run(['cmake', '--preset', profile])
        build = ['cmake', '--build', '--preset', profile, '--target',
                 *[prefix + stem for stem in stems]]
        run(build)
        directory = ROOT / ('build_modules_native' if imported else
                            'build_sanitize_current' if profile == 'sanitize' else 'build_native_current')
        binary = directory / ('modules' if imported else '') / (prefix + 'sqlite_chunk_capacity_probe')
        assert same(observe(binary, inputs), expected), profile
        junit = Path('/tmp') / (args.record.name + '.' + profile + '.xml')
        assert not junit.exists()
        run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
             '-R', '^(' + '|'.join(names) + ')$', '--output-junit', junit])
        xml = ET.fromstring(junit.read_text())
        assert sorted(case.attrib['name'] for case in xml.findall('.//testcase')) == sorted(names)
        assert not any(xml.findall('.//' + tag) for tag in ('failure', 'error', 'skipped'))
        # Configuring named modules can leave one metadata-only dyndep pass.
        stable = run(build)
        if 'ninja: no work to do.' not in stable:
            stable = run(build)
        assert 'ninja: no work to do.' in stable
        assert digest(ROOT / 'teaql_provider_sqlite/src/provider.cpp') == record['provider_sha256']
        record['profiles'][profile] = dict(tests=len(names), source_capacity_cases=858,
            retained_binding_cases=87, junit=junit.read_text(), probe_sha256=digest(binary))
        save()
        print(profile, len(names), 'checks; 858 capacities and 87 binding controls; no work', flush=True)

    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
