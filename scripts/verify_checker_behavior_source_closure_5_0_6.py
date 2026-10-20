#!/usr/bin/env python3
"""Replay unchanged Checker/assembly observers with reference dependencies."""
import argparse
import gzip
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
    parser.add_argument('--cpp-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    identities = registry_identities(reference)
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    hashes = {str(reference.relative_to(ROOT)): digest(reference)}
    local_domains = {'cpp-inheritance-core', 'cpp-inheritance-graph-core',
                     'cpp-inheritance-variants-core', 'cpp-checker-fixture-core',
                     'cpp-constants-numeric-core', 'cpp-relations-core'}
    for stem, package in (
            ('rust_generated_checker_closure_5_0_6', 'teaql_generated_checker_closure_506'),
            ('rust_generated_behavior_hooks_5_0_6', 'teaql_generated_behavior_hooks_506')):
        project = PARITY / stem
        lock = project / 'Cargo.lock'
        for name, resolved in versions(lock).items():
            if name not in {package, *local_domains} and not resolved <= expected.get(name, set()):
                raise RuntimeError(f'Checker dependency differs from reference: {name}: {resolved}')
        if not registry_identities(lock) <= identities:
            raise RuntimeError(f'Checker registry checksums differ: {stem}')
        subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                        str(project / 'Cargo.toml')], env=environment, check=True)
        for filename in ('Cargo.toml', 'Cargo.lock'):
            path = project / filename
            hashes[str(path.relative_to(ROOT))] = digest(path)
    matrices = (
        ('generated_checker_flow', 'compare_generated_checker_flow.py',
         'teaql_generated_checker_flow_probe', 'checker_flow_closure_506'),
        ('generated_checker_commit', 'compare_generated_checker_commit.py',
         'teaql_generated_checker_flow_probe', 'checker_flow_closure_506'),
        ('generated_runtime_assembly', 'compare_generated_runtime_assembly.py',
         'teaql_generated_runtime_assembly_probe', 'runtime_assembly_closure_506'),
        ('generated_behavior_hooks', 'compare_generated_behavior_hooks_5_0_6.py',
         'teaql_generated_behavior_hooks_probe', 'generated_behavior_hooks_506'),
    )
    results = []
    for stem, script, cpp, rust in matrices:
        corpus = PARITY / (stem + '_5_0_6_cases.json.gz')
        recorded = json.loads(gzip.decompress(corpus.read_bytes()))
        baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        if recorded['baseline'] != baseline:
            raise RuntimeError(f'Checker corpus baseline differs: {stem}')
        completed = subprocess.run([sys.executable, str(PARITY / script),
            '--cpp', str(args.cpp_dir / cpp), '--rust', str(target / 'debug' / rust)],
            env=environment, text=True, capture_output=True)
        print(completed.stdout, end='')
        if completed.returncode:
            print(completed.stderr, file=sys.stderr, end='')
            completed.check_returncode()
        results.append(dict(corpus=corpus.name, observations=len(recorded['cases']),
                            exit_code=completed.returncode, output=completed.stdout))
        for path in (corpus, PARITY / script):
            hashes[str(path.relative_to(ROOT))] = digest(path)
    for path in (Path(__file__),
            PARITY / 'rust_field_layout/src/bin/generated_checker_flow_5_0_6.rs',
            PARITY / 'rust_field_layout/src/bin/generated_runtime_assembly_5_0_6.rs',
            PARITY / 'generated_behavior_hooks_5_0_6.rs',
            PARITY / 'generated_behavior_hooks_probe.cpp'):
        hashes[str(path.relative_to(ROOT))] = digest(path)
    if args.record:
        Path(ROOT / 'docs/parity/checker_behavior_source_closure_5_0_6.json').write_text(
            json.dumps(dict(schema=1, runtime_commit=baseline,
                generator_commit=json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit'],
                observations=sum(r['observations'] for r in results), matrices=results,
                reference_registry_versions_preserved=True, reference_registry_checksums_preserved=True,
                original_generated_files_changed=False, existing_observers_changed=False,
                historical_manifests_changed=False, normalization='none',
                complete_capability_acceptance=False, hashes=hashes), indent=2) + '\n')
    print(f"Verified {sum(r['observations'] for r in results)} live Checker/Behavior/assembly outcomes")


if __name__ == '__main__':
    main()
