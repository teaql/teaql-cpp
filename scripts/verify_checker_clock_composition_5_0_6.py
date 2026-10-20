#!/usr/bin/env python3
"""Replay original configured composition entity/Checker bodies with reference dependencies."""
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
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_checker_clock_composition_probe')
    parser.add_argument('--clock', type=Path, default=ROOT / 'build_native_current/libteaql_sample_fixed_clock.so')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    project = ROOT / 'tests/parity/rust_checker_clock_composition_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name != 'teaql_checker_clock_composition_506' and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Composition clock dependency differs: {name}: {resolved}')
    if not registry_identities(project / 'Cargo.lock') <= registry_identities(reference):
        raise RuntimeError('Composition clock registry checksums differ')
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')],
                   env=environment, check=True)
    command = [sys.executable, str(ROOT / 'tests/parity/compare_checker_clock_composition_5_0_6.py'),
        '--cpp', str(args.cpp), '--clock', str(args.clock),
        '--rust', str(target / 'debug/teaql_checker_clock_composition_506')]
    if args.record:
        command.append('--record')
    result = subprocess.run(command, env=environment, text=True, capture_output=True)
    print(result.stdout, end='')
    if result.returncode:
        print(result.stderr, file=sys.stderr, end='')
        result.check_returncode()
    if args.record:
        paths = [Path(__file__), reference, project / 'Cargo.toml', project / 'Cargo.lock', project / 'lib.rs',
            ROOT / 'tests/parity/checker_clock_composition_5_0_6.rs',
            ROOT / 'tests/parity/checker_clock_composition_probe.cpp',
            ROOT / 'tests/parity/compare_checker_clock_composition_5_0_6.py',
            ROOT / 'tests/parity/checker_clock_composition_5_0_6_cases.json.gz',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/PROVENANCE.json',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/lib/src/note/entity.rs',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/lib/src/note/checker.rs',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/lib/src/record/entity.rs',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/lib/src/record/checker.rs',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/lib/src/tenant/entity.rs',
            ROOT / 'tests/generator/parity/checker_clock_composition_rust/lib/src/tenant/checker.rs']
        data = dict(schema=1, runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
            cases=216, field_outcomes=1296, comparison_exit_code=result.returncode, output=result.stdout,
            original_generated_files_changed=False, reference_registry_versions_preserved=True,
            reference_registry_checksums_preserved=True, normalization='none',
            source_scope='Original complete Tenant/Record/Note entity/Checker files imported by path; complete original crate '
                         'retains its missing module factory assembly error. No request/SQL/save acceptance claim.',
            complete_capability_acceptance=False,
            hashes={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
        (ROOT / 'docs/parity/checker_clock_composition_source_5_0_6.json').write_text(json.dumps(data, indent=2) + '\n')


if __name__ == '__main__':
    main()
