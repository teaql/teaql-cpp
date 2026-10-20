#!/usr/bin/env python3
"""Replay inheritance drivers unchanged with the exact reference dependency lock."""
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_state_baselines_5_0_6 import versions
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tests/parity'))
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
MATRICES = {
    'generated_inheritance': 'generated_inheritance',
    'generated_inheritance_query': 'generated_inheritance_query',
    'generated_inheritance_sqlite': 'generated_inheritance_sqlite',
    'generated_return_type': 'generated_inheritance_sqlite',
    'generated_inheritance_save': 'generated_inheritance_save',
    'generated_inheritance_graph': 'generated_inheritance_graph',
    'generated_inheritance_variants': 'generated_inheritance_variants',
    'generated_inherited_stream': 'generated_inherited_stream',
    'generated_inherited_json': 'generated_inherited_json',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    project = PARITY / 'rust_inheritance_closure_5_0_6'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    lock = project / 'Cargo.lock'
    expected = versions(reference_lock)
    local = {'teaql_inheritance_closure_506', 'cpp-inheritance-core',
             'cpp-inheritance-graph-core', 'cpp-inheritance-variants-core'}
    for name, resolved in versions(lock).items():
        if name not in local and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Inheritance observer dependency differs: {name}: {resolved}')
    def identities(path):
        return {tuple(re.search(rf'^{field} = "([^"]+)"', block, re.M)[1]
                      for field in ('name', 'version', 'source', 'checksum'))
                for block in path.read_text().split('[[package]]')[1:]
                if '\nsource = "registry+' in block}
    if not identities(lock) <= identities(reference_lock):
        raise RuntimeError('Inheritance observer dependency checksums differ')
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(project / 'Cargo.toml'), '--bins'], env=environment, check=True)
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    records = []
    for matrix, driver in MATRICES.items():
        corpus_path = PARITY / (matrix + '_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        inputs = [case['input'] for case in corpus['cases']]
        recorded = [case['expected'] for case in corpus['cases']]
        source = observe(str(target / 'debug' / (driver + '_5_0_6')), inputs)
        cpp_name = 'generated_inheritance_sqlite' if matrix == 'generated_return_type' else matrix
        native = observe(str(ROOT / 'build_native_current' / ('teaql_' + cpp_name + '_probe')), inputs)
        gaps = [dict(index=index, input=value, recorded=old, rust=actual, cpp=cpp)
                for index, (value, old, actual, cpp) in enumerate(zip(inputs, recorded, source, native))
                if not same(old, actual) or not same(actual, cpp)]
        if gaps:
            path = ROOT / 'docs/parity/inheritance_closure_5_0_6_initial_gaps.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress(json.dumps(dict(matrix=matrix, gaps=gaps), indent=2)
                                               .encode(), mtime=0))
            raise RuntimeError(f'{matrix}: {len(gaps)} fresh source/native differences; see {path}')
        driver_path = PARITY / 'rust_field_layout/src/bin' / (driver + '_5_0_6.rs')
        records.append(dict(matrix=matrix, observations=len(inputs),
                            corpus_sha256=digest(corpus_path), unchanged_driver=str(driver_path.relative_to(ROOT)),
                            unchanged_driver_sha256=digest(driver_path)))
        print(f'{matrix}: {len(inputs)} exact reference-locked source/native outcomes', flush=True)
    record = dict(schema=1, baseline=corpus['baseline'], matrices=records,
                  observations=sum(row['observations'] for row in records),
                  manifest_sha256=digest(project / 'Cargo.toml'), lock_sha256=digest(lock),
                  reference_lock_sha256=digest(reference_lock),
                  source_dependency_versions_preserved=True, source_dependency_checksums_preserved=True,
                  original_drivers_changed=False, original_generated_source_changed=False,
                  normalization='none', complete_capability_acceptance=False)
    (ROOT / 'docs/parity/inheritance_closure_source_5_0_6.json').write_text(
        json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
