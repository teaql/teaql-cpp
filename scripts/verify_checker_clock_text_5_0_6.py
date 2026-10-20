#!/usr/bin/env python3
"""Build and replay unchanged generated Rust string-clock Checker methods."""
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_checker_clock_text_probe')
    parser.add_argument('--clock', type=Path, default=ROOT / 'build_native_current/libteaql_sample_fixed_clock.so')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    project = ROOT / 'tests/parity/rust_checker_clock_text_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name not in {'teaql_checker_clock_text_506', 'cpp-checker-unique-string-core'} and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'String clock observer dependency differs: {name}: {resolved}')
    if not registry_identities(project / 'Cargo.lock') <= registry_identities(reference):
        raise RuntimeError('String clock observer registry checksums differ')
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')],
                   env=environment, check=True)
    command = [sys.executable, str(ROOT / 'tests/parity/compare_checker_clock_text_5_0_6.py'),
               '--cpp', str(args.cpp), '--clock', str(args.clock),
               '--rust', str(target / 'debug/teaql_checker_clock_text_506')]
    if args.record:
        command.append('--record')
    result = subprocess.run(command, env=environment, text=True, capture_output=True)
    print(result.stdout, end='')
    if result.returncode:
        print(result.stderr, file=sys.stderr, end='')
        result.check_returncode()
    if args.record:
        paths = [Path(__file__), reference, project / 'Cargo.toml', project / 'Cargo.lock',
            ROOT / 'tests/parity/checker_clock_text_5_0_6.rs',
            ROOT / 'tests/parity/checker_clock_text_probe.cpp',
            ROOT / 'tests/parity/compare_checker_clock_text_5_0_6.py',
            ROOT / 'tests/parity/checker_clock_text_5_0_6_cases.json.gz',
            ROOT / 'tests/generator/parity/checker_unique_string_rust/PROVENANCE.json']
        data = dict(schema=1, runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
            observations=48, comparison_exit_code=result.returncode, output=result.stdout,
            original_generated_files_changed=False, reference_registry_versions_preserved=True,
            reference_registry_checksums_preserved=True, normalization='none',
            complete_capability_acceptance=False,
            hashes={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
        (ROOT / 'docs/parity/checker_clock_text_source_5_0_6.json').write_text(json.dumps(data, indent=2) + '\n')


if __name__ == '__main__':
    main()
