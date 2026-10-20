#!/usr/bin/env python3
"""Replay unchanged constant observers with the exact reference dependency lock.

Source outputs must match their existing corpora before the original comparators
validate C++. The resource comparator retains its explicit destruction-timing
adaptation; this script does not turn that adaptation into raw-output equality.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same

MATRICES = (
    ('generated_candidates', 'candidates', 'generated_candidates',
     'compare_generated_candidates.py'),
    ('generated_bootstrap', 'bootstrap', 'generated_bootstrap',
     'compare_generated_bootstrap.py'),
    ('generated_constant_variants', 'variants', 'generated_constant_variants',
     'compare_generated_constant_variants.py'),
    ('constant_governance', 'variants', 'generated_constant_variants',
     'compare_constant_governance_5_0_6.py'),
    ('bootstrap_extensions', 'extensions', 'bootstrap_extensions',
     'compare_bootstrap_extensions_5_0_6.py'),
    ('bootstrap_conflict', 'conflict', 'bootstrap_conflict',
     'compare_bootstrap_conflict_5_0_6.py'),
    ('resource_bindings', 'resources', 'resource_bindings',
     'compare_resource_bindings_5_0_6.py'),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def registry_identities(path):
    return {tuple(re.search(rf'^{field} = "([^"]+)"', block, re.M)[1]
                  for field in ('name', 'version', 'source', 'checksum'))
            for block in path.read_text().split('[[package]]')[1:]
            if '\nsource = "registry+' in block}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', action='store_true',
                        help='Save a new closure evidence record after all checks pass')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    project = PARITY / 'rust_constant_closure_5_0_6'
    lock = project / 'Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    local = {'teaql_constant_closure_506', 'cpp-candidates-core',
             'cpp-constants-numeric-core', 'cpp-constants-catalog-core'}
    expected = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in local and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Constant observer dependency differs: {name}: {resolved}')
    if not registry_identities(lock) <= registry_identities(reference_lock):
        raise RuntimeError('Constant observer dependency checksums differ')
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    drivers = {re.search(r'^name = "([^"]+)"', block, re.M)[1]:
               (project / re.search(r'^path = "([^"]+)"', block, re.M)[1]).resolve()
               for block in (project / 'Cargo.toml').read_text().split('[[bin]]')[1:]}
    driver_hashes = {name: digest(path) for name, path in drivers.items()}
    provenance = {}
    for name in ('candidates', 'constants_numeric', 'constants_catalog'):
        directory = ROOT / 'tests/generator/parity' / (name + '_rust')
        path = directory / 'PROVENANCE.json'
        data = json.loads(path.read_text())
        files = {p.relative_to(directory).as_posix(): digest(p)
                 for p in directory.rglob('*')
                 if p.is_file() and p.name != 'PROVENANCE.json'}
        if data['hand_edited'] or data['runtime_commit'] != baseline or files != data['files']:
            raise RuntimeError(f'Original generated constant SDK differs: {name}')
        if digest(ROOT / data['model']) != data['model_sha256']:
            raise RuntimeError(f'Original model input differs: {name}')
        provenance[name] = digest(path)
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(project / 'Cargo.toml'), '--bins'], env=environment, check=True)
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    records = []
    for matrix, driver, native, comparator in MATRICES:
        corpus_path = PARITY / (matrix + '_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
        if corpus['baseline'] != baseline:
            raise RuntimeError(f'{matrix}: corpus baseline differs')
        cases = corpus['cases']
        executable = target / 'debug' / ('constant_' + driver + '_closure_506')
        actual = observe(str(executable), [case['input'] for case in cases])
        gaps = [dict(index=i, input=case['input'], recorded=case['expected'], rust=value)
                for i, (case, value) in enumerate(zip(cases, actual))
                if not same(case['expected'], value)]
        if gaps:
            path = ROOT / 'docs/parity/constant_closure_5_0_6_source_gaps.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress(json.dumps(dict(matrix=matrix, gaps=gaps), indent=2)
                                               .encode(), mtime=0))
            raise RuntimeError(f'{matrix}: {len(gaps)} reference-locked source differences; see {path}')
        subprocess.run([sys.executable, str(PARITY / comparator), '--cpp',
                        str(args.cpp_dir / ('teaql_' + native + '_probe'))],
                       env=environment, check=True)
        name = 'constant_' + driver + '_closure_506'
        records.append(dict(matrix=matrix, observations=len(cases),
                            corpus_sha256=digest(corpus_path),
                            unchanged_driver=str(drivers[name].relative_to(ROOT)),
                            unchanged_driver_sha256=driver_hashes[name],
                            source_recorded_outputs_equal=True, comparator=comparator,
                            comparator_sha256=digest(PARITY / comparator),
                            cpp_exit_code=0))
        print(f'{matrix}: {len(cases)} reference-locked source observations and C++ contracts passed',
              flush=True)
    if driver_hashes != {name: digest(path) for name, path in drivers.items()}:
        raise RuntimeError('Observer drivers changed during closure replay')
    record = dict(schema=1, baseline=baseline, matrices=records,
                  observations=sum(row['observations'] for row in records),
                  manifest_sha256=digest(project / 'Cargo.toml'), lock_sha256=digest(lock),
                  reference_lock_sha256=digest(reference_lock), original_sdk_provenance=provenance,
                  source_dependency_versions_preserved=True, source_dependency_checksums_preserved=True,
                  original_drivers_changed=False, original_generated_source_changed=False,
                  resource_ownership_adaptation='Rust replacement Drop is immediate; C++ arena destruction occurs at Context close. Existing comparator checks each timing contract separately.',
                  complete_capability_acceptance=False)
    if args.record:
        (ROOT / 'docs/parity/constant_closure_source_5_0_6.json').write_text(
            json.dumps(record, indent=2) + '\n')
    print(f'Constant closure: {record["observations"]} source observations; seven C++ matrices passed')


if __name__ == '__main__':
    main()
