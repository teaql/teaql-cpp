#!/usr/bin/env python3
"""Build and replay the original inherited domain with reference dependencies."""
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


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--clock', type=Path,
                        default=ROOT / 'build_native_current/libteaql_sample_fixed_clock.so')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    project = ROOT / 'tests/parity/rust_inherited_checker_helpers_5_0_6'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    lock = project / 'Cargo.lock'
    local = {'teaql_inherited_checker_helpers_506', 'cpp-inheritance-core',
             'cpp-inheritance-graph-core', 'cpp-inheritance-variants-core'}
    expected = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in local and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Inherited Checker dependency changed: {name}: {resolved}')
    if not registry_identities(lock) <= registry_identities(reference_lock):
        raise RuntimeError('Inherited Checker registry checksums changed')
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(project / 'Cargo.toml')], env=environment, check=True)
    subprocess.run([sys.executable, str(ROOT / 'tests/parity/compare_inherited_checker_helpers_5_0_6.py'),
                    '--cpp-dir', str(args.cpp_dir), '--clock', str(args.clock),
                    '--rust', str(target / 'debug/inherited_checker_helpers_506')],
                   env=environment, check=True)
    print('Reference dependency versions/checksums and live inherited Checker replay verified')


if __name__ == '__main__':
    main()
