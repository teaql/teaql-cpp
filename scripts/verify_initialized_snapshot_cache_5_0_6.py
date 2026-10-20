#!/usr/bin/env python3
"""Replay unchanged original public cache APIs with reference dependencies."""
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
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_initialized_snapshot_cache_probe')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    project = ROOT / 'tests/parity/rust_initialized_snapshot_cache_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name != 'teaql_initialized_snapshot_cache_506' and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Cache dependency differs from reference: {name}: {resolved}')
    if not registry_identities(project / 'Cargo.lock') <= registry_identities(reference):
        raise RuntimeError('Cache registry checksums differ')
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(project / 'Cargo.toml')], env=environment, check=True)
    command = [sys.executable, str(ROOT / 'tests/parity/compare_initialized_snapshot_cache_5_0_6.py'),
               '--cpp', str(args.cpp), '--rust', str(target / 'debug/teaql_initialized_snapshot_cache_506')]
    if args.record:
        command.append('--record')
    result = subprocess.run(command, env=environment, text=True, capture_output=True)
    print(result.stdout, end='')
    if result.returncode:
        print(result.stderr, file=sys.stderr, end='')
        result.check_returncode()
    if args.record:
        paths = [Path(__file__), reference, project / 'Cargo.toml', project / 'Cargo.lock',
                 ROOT / 'tests/parity/initialized_snapshot_cache_5_0_6.rs',
                 ROOT / 'tests/parity/initialized_snapshot_cache_probe.cpp',
                 ROOT / 'tests/parity/compare_initialized_snapshot_cache_5_0_6.py',
                 ROOT / 'tests/parity/initialized_snapshot_cache_5_0_6_cases.json.gz']
        for filename in ('load_state.rs', 'query.rs', 'dynamic_properties.rs'):
            paths.append(ROOT / '.reference/teaql-rs/teaql-core/src' / filename)
        record = dict(schema=1, runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                      source_scope='Unchanged original public core APIs; no private bridge, local implementation or source rewrite.',
                      exact_semantic_cases=80, ownership_adaptations=1, comparison_exit_code=0,
                      output=result.stdout, reference_registry_versions_preserved=True,
                      reference_registry_checksums_preserved=True,
                      diagnostics='Actual raw original/native initialized output retained independently; randomized/address-bearing text is not compared or normalized.',
                      complete_capability_acceptance=False,
                      hashes={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
        (ROOT / 'docs/parity/initialized_snapshot_cache_source_5_0_6.json').write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
