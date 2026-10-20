#!/usr/bin/env python3
"""Replay SQLite lease/binding source contracts and selected compiled consumers."""
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
PROJECT = PARITY / 'rust_sqlite_lease_binding_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('sqlite_lease_binding_506', 'sqlite_lease_binding', 15),
    ('sqlite_lease_binding_guard_506', 'sqlite_lease_binding_guard', 72),
)
BASE_NAMES = ['sqlite_lease_binding_rust_parity', 'sqlite_lease_binding_guard_rust_parity',
              'provider_sqlite_transaction_stream_contracts']
BASE_STEMS = ['sqlite_lease_binding_probe', 'sqlite_async_lease_probe',
              'native_transaction_stream_tests']


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
        parser.error('Use a fresh receipt path; preserve prior evidence')
    record = dict(complete=False, full_provider_accepted=False, full_goal_accepted=False,
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
        values = [json.loads(line) for line in output.split('\n') if line]
        assert len(values) == len(inputs)
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(
        ROOT / '.reference/teaql-rs/Cargo.lock')
    record['manifest_sha256'] = digest(PROJECT / 'Cargo.toml')
    record['lock_sha256'] = digest(PROJECT / 'Cargo.lock')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bins', '-j', '2'])
    reference = {}
    for stem, name, count in MATRICES:
        path = PARITY / (name + '_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(path.read_bytes()))
        assert len(corpus['cases']) == count
        assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        assert digest(ROOT / corpus['original_driver']) == corpus['original_driver_sha256']
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        original = observe(Path(env['CARGO_TARGET_DIR']) / 'debug' / stem, inputs)
        assert same(original, expected), name
        reference[name] = (inputs, original)
    record['source_outcomes'] = 87
    provider = ROOT / 'teaql_provider_sqlite/src/provider.cpp'
    record['provider_sha256'] = digest(provider)

    for profile in args.profiles:
        imported = profile == 'modules_native'
        prefix = 'teaql_module_' if imported else 'teaql_'
        case_prefix = 'module_' if imported else ''
        stems, names = list(BASE_STEMS), list(BASE_NAMES)
        if profile == 'native':
            stems += ['provider_sqlite_transport_tests']
            names += ['provider_sqlite_transport_contracts', 'sqlite_async_lease_rust_parity']
            for suffix in ('storage', 'schema', 'query', 'transaction'):
                stems += ['original_provider_' + suffix + '_tests']
                names += ['original_provider_' + suffix + '_contracts']
        elif imported:
            stems += ['provider_sqlite_transport_tests']
            names += ['provider_sqlite_transport_contracts']
        else:
            names += ['sqlite_async_lease_rust_parity']
        names = [case_prefix + name for name in names]
        run(['cmake', '--preset', profile])
        build = ['cmake', '--build', '--preset', profile, '--target',
                 *[prefix + stem for stem in stems]]
        run(build)
        directory = ROOT / ('build_modules_native' if imported else
                            'build_sanitize_current' if profile == 'sanitize' else 'build_native_current')
        binary = directory / ('modules' if imported else '') / (prefix + 'sqlite_lease_binding_probe')
        for name, (inputs, expected) in reference.items():
            assert same(observe(binary, inputs), expected), (profile, name)
        junit = Path('/tmp') / (args.record.name + '.' + profile + '.xml')
        assert not junit.exists()
        run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
             '-R', '^(' + '|'.join(names) + ')$', '--output-junit', junit])
        xml = ET.fromstring(junit.read_text())
        assert sorted(case.attrib['name'] for case in xml.findall('.//testcase')) == sorted(names)
        assert not any(xml.findall('.//' + tag) for tag in ('failure', 'error', 'skipped'))
        if imported:
            run(['python3', PARITY / 'compare_sqlite_async_lease_5_0_6.py', '--cpp',
                 directory / 'modules/teaql_module_sqlite_async_lease_probe'])
        assert 'ninja: no work to do.' in run(build)
        assert digest(provider) == record['provider_sha256']
        record['profiles'][profile] = dict(tests=len(names), source_outcomes=231,
            junit=junit.read_text(), probe_sha256=digest(binary),
            original_provider_root_ports=42 if profile == 'native' else 0,
            asynchronous_lease_comparator='standalone' if imported else 'CTest')
        save()
        print(profile, len(names), 'checks and 231 exact source outcomes; no work', flush=True)

    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
