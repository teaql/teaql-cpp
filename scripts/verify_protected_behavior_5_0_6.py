#!/usr/bin/env python3
"""Verify complete original source bytes, reference dependencies and live replay."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_protected_behavior_probe')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    source = ROOT / 'build_rust_protected_behavior_5_0_6/original'
    provenance = json.loads((source.parent / 'provenance.json').read_text())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if provenance['runtime_commit'] != baseline:
        raise RuntimeError('Derived protected runtime belongs to a different baseline')
    for path, record in provenance['files'].items():
        original = ROOT / '.reference/teaql-rs' / path
        original_bytes = original.read_bytes()
        suffix = provenance['bridge'].encode() if path == 'teaql-runtime/src/data_service/resolved.rs' else b''
        actual = source / path
        if actual.read_bytes() != original_bytes + suffix or digest(actual) != record['compiled_sha256']:
            raise RuntimeError(f'Original source body changed: {path}')
        if digest(original) != record['original_sha256']:
            raise RuntimeError(f'Original source fingerprint changed: {path}')
    project = PARITY / 'rust_protected_behavior_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    lock = project / 'Cargo.lock'
    for name, resolved in versions(lock).items():
        if name not in {'teaql_protected_behavior_506', 'cpp-checker-fixture-core'} and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Protected Behavior dependency differs: {name}: {resolved}')
    if not registry_identities(lock) <= registry_identities(reference):
        raise RuntimeError('Protected Behavior registry checksums differ')
    target = Path('/tmp/teaql_cpp_protected_behavior_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--locked', '--offline', '--manifest-path', str(project / 'Cargo.toml')],
                   env=environment, check=True)
    result = subprocess.run([sys.executable, str(PARITY / 'compare_protected_behavior_5_0_6.py'),
                            '--cpp', str(args.cpp.resolve()), '--rust', str(target / 'debug/protected_behavior_506')],
                            env=environment, text=True, capture_output=True)
    print(result.stdout, end='')
    if result.returncode:
        print(result.stderr, file=sys.stderr, end='')
        result.check_returncode()
    if args.record:
        files = [Path(__file__), PARITY / 'build_protected_behavior_oracle_5_0_6.py',
                 PARITY / 'protected_behavior_5_0_6.rs', PARITY / 'protected_behavior_probe.cpp',
                 PARITY / 'compare_protected_behavior_5_0_6.py',
                 PARITY / 'protected_behavior_5_0_6_cases.json.gz',
                 project / 'Cargo.toml', lock, reference,
                 ROOT / 'tests/generator/parity/checkers_rust/PROVENANCE.json']
        (ROOT / 'docs/parity/protected_behavior_source_5_0_6.json').write_text(json.dumps(dict(
            schema=1, runtime_commit=baseline, original_source_files=len(provenance['files']),
            appended_source_files=1, original_method_bodies_changed=False,
            reference_registry_versions_preserved=True, reference_registry_checksums_preserved=True,
            observer_exit_code=result.returncode, output=result.stdout,
            bridge=provenance['bridge'],
            original_resolved_sha256=provenance['files']['teaql-runtime/src/data_service/resolved.rs']['original_sha256'],
            compiled_resolved_sha256=provenance['files']['teaql-runtime/src/data_service/resolved.rs']['compiled_sha256'],
            hashes={str(path.relative_to(ROOT)): digest(path) for path in files}), indent=2) + '\n')
    print('Complete original runtime bytes and protected SQLite replay verified')


if __name__ == '__main__':
    main()
