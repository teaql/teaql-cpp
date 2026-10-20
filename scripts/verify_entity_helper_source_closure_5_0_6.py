#!/usr/bin/env python3
"""Replay unchanged entity observers with the current reference dependencies."""
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
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference)
    expected_registry = registry_identities(reference)
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    projects = (
        ('rust_entity_helper_closure_5_0_6', 'teaql_entity_helper_closure_506'),
        ('rust_recursive_attachment_helpers_5_0_6', 'teaql_recursive_attachment_helpers_506'),
    )
    hashes = {str(reference.relative_to(ROOT)): digest(reference)}
    for directory, package in projects:
        project = PARITY / directory
        lock = project / 'Cargo.lock'
        for name, resolved in versions(lock).items():
            if name not in {package, 'cpp-relations-core'} and not resolved <= expected_versions.get(name, set()):
                raise RuntimeError(f'Entity helper dependency changed: {name}: {resolved}')
        if not registry_identities(lock) <= expected_registry:
            raise RuntimeError(f'Entity helper registry checksums changed: {directory}')
        subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                        str(project / 'Cargo.toml')], env=environment, check=True)
        for name in ('Cargo.toml', 'Cargo.lock'):
            path = project / name
            hashes[str(path.relative_to(ROOT))] = digest(path)
    matrices = (
        ('compare_generated_entity_helpers.py', 'teaql_generated_entity_helpers_probe',
         'entity_helpers_506', 'generated_entity_helpers_5_0_6_cases.json', 1624),
        ('compare_generated_optional_fields.py', 'teaql_generated_entity_helpers_probe',
         'entity_helpers_506', 'generated_optional_fields_5_0_6_cases.json.gz', 1548),
        ('compare_recursive_attachment_helpers_5_0_6.py', 'teaql_recursive_attachment_helpers_probe',
         'recursive_attachment_helpers_506', 'recursive_attachment_helpers_5_0_6_cases.json.gz', 192),
    )
    results = []
    for script, cpp, rust, corpus, count in matrices:
        corpus_path = PARITY / corpus
        raw = corpus_path.read_bytes()
        recorded = json.loads(gzip.decompress(raw) if corpus_path.suffix == '.gz' else raw)
        baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        if recorded['baseline'] != baseline or len(recorded['cases']) != count:
            raise RuntimeError(f'Entity helper corpus baseline/count differs: {corpus}')
        command = [sys.executable, str(PARITY / script), '--cpp', str(args.cpp_dir / cpp),
                   '--rust', str(target / 'debug' / rust)]
        completed = subprocess.run(command, env=environment, text=True, capture_output=True)
        print(completed.stdout, end='')
        if completed.returncode:
            print(completed.stderr, file=sys.stderr, end='')
            completed.check_returncode()
        results.append(dict(corpus=corpus, observations=count, exit_code=completed.returncode,
                            output=completed.stdout))
        for path in (PARITY / script, PARITY / corpus):
            hashes[str(path.relative_to(ROOT))] = digest(path)
    for path in (
            Path(__file__), PARITY / 'rust_types_entities/src/lib.rs',
            PARITY / 'rust_field_layout/src/bin/generated_entity_helpers_5_0_6.rs',
            PARITY / 'recursive_attachment_helpers_5_0_6.rs',
            PARITY / 'generated_entity_helpers_probe.cpp',
            PARITY / 'recursive_attachment_helpers_probe.cpp',
            ROOT / 'tests/generator/parity/types_rust/PROVENANCE.json',
            ROOT / 'tests/generator/parity/optional_types_rust/PROVENANCE.json',
            ROOT / 'tests/generator/parity/relations_rust/PROVENANCE.json'):
        hashes[str(path.relative_to(ROOT))] = digest(path)
    if args.record:
        output = ROOT / 'docs/parity/entity_helper_source_closure_5_0_6.json'
        output.write_text(json.dumps(dict(
            schema=1, runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
            generator_commit=json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit'],
            observations=sum(row['observations'] for row in results), matrices=results,
            reference_registry_versions_preserved=True, reference_registry_checksums_preserved=True,
            original_entity_checker_files_changed=False, existing_entity_observer_changed=False,
            recursive_observer_added=True,
            historical_manifests_changed=False, normalization='none',
            access_adaptation='Unchanged entity/Checker-only path harness excludes broken original request/runtime scaffolds; relations use the complete original crate.',
            complete_generator_acceptance=False, hashes=hashes), indent=2) + '\n')
    print('Reference dependency versions/checksums and 3364 live entity helper outcomes verified')


if __name__ == '__main__':
    main()
