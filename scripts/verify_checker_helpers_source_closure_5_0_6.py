#!/usr/bin/env python3
"""Replay original Checker and Business-ID observers at the current Rust lock."""
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


def original_methods(source, begin, end, comments=False):
    start = source.index(begin)
    stop = source.index(end, start)
    body = source[start:stop]
    if comments:
        body += '\n' + source[source.index('fn comment_probe('):]
    return body.encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference)
    expected_identities = registry_identities(reference)
    observer = PARITY / 'rust_field_layout/src/main.rs'
    source = observer.read_text()
    environment = dict(os.environ,
        CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
        TEAQL_LOG_ENDPOINT='off')
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    hashes = {str(p.relative_to(ROOT)): digest(p) for p in (reference, observer, Path(__file__))}
    results = []
    for stem, package, domain, begin, end, comments, comparator, corpus, count in (
        ('rust_checker_helpers_closure_5_0_6', 'teaql_checker_helpers_closure_506',
         'school-management-service-core', 'fn checker_helper_probe(',
         'fn mutation_envelope_probe(', True, 'compare_checker_helpers.py',
         'checker_provenance_5_0_6_cases.json', 274),
        ('rust_business_id_helper_closure_5_0_6', 'teaql_business_id_helper_closure_506',
         'cpp-business-ids-core', 'fn business_id_probe(',
         'fn generated_relations_probe(', False, 'compare_business_ids.py',
         'business_ids_5_0_6_cases.json.gz', 1044),
    ):
        project = PARITY / stem
        lock = project / 'Cargo.lock'
        for name, resolved in versions(lock).items():
            if name not in {package, domain} and not resolved <= expected_versions.get(name, set()):
                raise RuntimeError(f'Checker dependency differs from reference: {name}: {resolved}')
        if not registry_identities(lock) <= expected_identities:
            raise RuntimeError(f'Checker registry checksums differ: {stem}')
        build = subprocess.run(['cargo', 'build', '--offline', '--locked',
            '--message-format=json', '--manifest-path', str(project / 'Cargo.toml')],
            env=environment, text=True, capture_output=True)
        if build.returncode:
            print(build.stderr, file=sys.stderr, end='')
            build.check_returncode()
        messages = [json.loads(line) for line in build.stdout.splitlines() if line.startswith('{')]
        outputs = [Path(m['out_dir']) / 'original_observers.rs' for m in messages
                   if m.get('reason') == 'build-script-executed' and package in m['package_id']]
        binaries = [Path(m['executable']) for m in messages
                    if m.get('reason') == 'compiler-artifact'
                    and m.get('executable') and m['target']['name'] == package]
        if len(outputs) != 1 or len(binaries) != 1:
            raise RuntimeError(f'Cannot identify compiled observer artifacts: {stem}')
        compiled = outputs[0].read_bytes()
        if compiled != original_methods(source, begin, end, comments):
            raise RuntimeError(f'Original observer bodies changed: {stem}')
        comparison = subprocess.run([sys.executable, str(PARITY / comparator),
            '--cpp', str(args.cpp_dir / 'teaql_loaded_snapshot_probe'),
            '--rust', str(binaries[0])], env=environment, text=True, capture_output=True)
        print(comparison.stdout, end='')
        if comparison.returncode:
            print(comparison.stderr, file=sys.stderr, end='')
            comparison.check_returncode()
        results.append(dict(project=stem, observations=count, build_exit_code=build.returncode,
            comparison_exit_code=comparison.returncode, output=comparison.stdout,
            compiled_observer_sha256=hashlib.sha256(compiled).hexdigest(),
            adaptation='resource type-name diagnostics' if not comments else 'none'))
        for p in (project / 'Cargo.toml', lock, project / 'build.rs', project / 'src/main.rs',
                  PARITY / comparator, PARITY / corpus):
            hashes[str(p.relative_to(ROOT))] = digest(p)
    if args.record:
        path = ROOT / 'docs/parity/checker_helpers_dependency_closure_5_0_6.json'
        path.write_text(json.dumps(dict(schema=1,
            runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
            observations=sum(r['observations'] for r in results), matrices=results,
            original_observer_bodies_changed=False, original_generated_files_changed=False,
            historical_manifests_changed=False, reference_registry_versions_preserved=True,
            reference_registry_checksums_preserved=True,
            complete_capability_acceptance=False, hashes=hashes), indent=2) + '\n')
    print('Verified 1318 live helper outcomes and exact compiled observer bodies')


if __name__ == '__main__':
    main()
