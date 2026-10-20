#!/usr/bin/env python3
"""Verify the accepted source binding adaptation and custom governed saves."""
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
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_inherited_business_extensions_probe')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    subprocess.run([sys.executable, str(PARITY / 'build_inherited_business_binding_bridge_5_0_6.py')], check=True)
    bridge = ROOT / 'build_inherited_business_binding_bridge_5_0_6/provenance.json'
    provenance = json.loads(bridge.read_text())
    project = PARITY / 'rust_inherited_business_extensions_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    lock = project / 'Cargo.lock'
    expected = versions(reference)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_inherited_business_extensions_506', 'cpp-inheritance-business-ids-core'} and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Inherited extension dependency differs: {name}: {resolved}')
    if not registry_identities(lock) <= registry_identities(reference):
        raise RuntimeError('Inherited extension registry checksums differ')
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off', TEAQL_BUSINESS_ID_KEY_VERSION='1',
                       TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')],
                   env=environment, check=True)
    result = subprocess.run([sys.executable, str(PARITY / 'compare_inherited_business_extensions_5_0_6.py'),
                            '--cpp', str(args.cpp.resolve()),
                            '--rust', str(target / 'debug/inherited_business_extensions_506')],
                            env=environment, text=True, capture_output=True)
    print(result.stdout, end='')
    if result.returncode:
        print(result.stderr, file=sys.stderr, end='')
        result.check_returncode()
    if args.record:
        files = [Path(__file__), PARITY / 'inherited_business_extensions_5_0_6.rs',
                 PARITY / 'inherited_business_extensions_probe.cpp',
                 PARITY / 'compare_inherited_business_extensions_5_0_6.py',
                 PARITY / 'inherited_business_extensions_5_0_6_cases.json.gz',
                 PARITY / 'build_inherited_business_binding_bridge_5_0_6.py',
                 project / 'Cargo.toml', lock, reference,
                 ROOT / 'tests/generator/parity/inheritance_business_ids_rust/PROVENANCE.json']
        (ROOT / 'docs/parity/inherited_business_extensions_source_5_0_6.json').write_text(json.dumps(dict(
            schema=1, runtime_commit=provenance['runtime_commit'], generator_commit=provenance['generator_commit'],
            adapted_source_only=True, untouched_producer_acceptance=False,
            original_files_unchanged=True, reference_registry_versions_preserved=True,
            reference_registry_checksums_preserved=True, adaptation=provenance['adaptation'],
            derived_provenance=provenance, observer_exit_code=result.returncode, output=result.stdout,
            hashes={str(path.relative_to(ROOT)): digest(path) for path in files}), indent=2) + '\n')
    print('Accepted inherited binding adaptation and live custom governed saves verified')


if __name__ == '__main__':
    main()
