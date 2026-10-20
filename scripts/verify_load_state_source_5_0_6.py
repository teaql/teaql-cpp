#!/usr/bin/env python3
"""Replay the complete indexed-state source review with reference dependencies."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same
from compare_initialized_snapshot_cache_5_0_6 import cases as cache_cases, validate_ownership

FUNCTIONS = ('strings', 'evaluate', 'data_type', 'native_value', 'schema',
             'schema_probe', 'snapshot_probe', 'load_state_probe', 'row_probe')
MATRICES = (
    ('field_layout', 'field_layout_5_0_6_cases.json', 'load_state_reference_5_0_6', 44),
    ('loaded_snapshot', 'loaded_snapshot_5_0_6_cases.json', 'load_state_reference_5_0_6', 427),
    ('dynamic_carrier', 'dynamic_carrier_5_0_6_cases.json', 'dynamic_carrier_reference_5_0_6', 176),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def functions(source):
    result = {}
    for name in FUNCTIONS:
        matches = re.findall(rf'(?ms)^fn {name}\(.*?(?=^fn |\Z)', source)
        if len(matches) != 1:
            raise RuntimeError(f'Missing or duplicate observation function: {name}')
        result[name] = matches[0].rstrip() + '\n'
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust-dir', type=Path,
                        default=Path('/tmp/teaql_cpp_core_baselines_506/debug'))
    parser.add_argument('--cache-rust', type=Path, default=Path(
        '/tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_initialized_snapshot_cache_506'))
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    parser.add_argument('--implementation-review', type=Path, default=ROOT /
                        'docs/parity/g15_current_responsibility_review_5_0_6.json.gz')
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists():
        parser.error('Preserve previous evidence; select a fresh receipt path')
    record = dict(scope='Whole load_state.rs production and its two lifetime tests; '
                       'not all adjacent eval, row, dynamic-field or integration-test sources',
                  complete=False, matrices=[], implementation_hashes={}, observer_hashes={})

    def save():
        args.receipt.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    result = subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')],
                            cwd=ROOT, capture_output=True, text=True)
    record['upstream_verification'] = dict(exit_code=result.returncode,
                                          stdout=result.stdout, stderr=result.stderr)
    save()
    result.check_returncode()
    lock = read(ROOT / 'upstream.lock.json')
    if lock['version'] != '5.0.6':
        raise RuntimeError('This review requires the locked 5.0.6 reference')
    record['baseline'] = lock
    reference = ROOT / '.reference/teaql-rs'
    source = 'teaql-core/src/load_state.rs'
    expected = next(item for item in read(ROOT / 'docs/parity/v5_0_6/source_dispositions.json')['sources']
                    if item['source'] == source)['sha256']
    if digest(reference / source) != expected:
        raise RuntimeError('The reviewed production source changed')
    record['source'] = dict(path=source, sha256=expected,
                            complete_source=(reference / source).read_text())
    project = PARITY / 'rust_core_primitives_reference_5_0_6'
    cache_project = PARITY / 'rust_initialized_snapshot_cache_5_0_6'
    reference_registry = registry_identities(reference / 'Cargo.lock')
    for workspace in (project, cache_project):
        if not registry_identities(workspace / 'Cargo.lock') <= reference_registry:
            raise RuntimeError(f'Observer registry identity drift: {workspace}')
        record['observer_hashes'][str((workspace / 'Cargo.toml').relative_to(ROOT))] = digest(workspace / 'Cargo.toml')
        record['observer_hashes'][str((workspace / 'Cargo.lock').relative_to(ROOT))] = digest(workspace / 'Cargo.lock')
    original_driver = PARITY / 'rust_field_layout/src/main.rs'
    driver = project / 'src/bin/load_state_reference_5_0_6.rs'
    historical = functions(original_driver.read_text())
    if historical != functions(driver.read_text()):
        raise RuntimeError('An isolated observation function changed')
    record['preserved_observer_functions'] = {
        name: hashlib.sha256(body.encode()).hexdigest() for name, body in historical.items()}
    include = project / 'src/bin/dynamic_carrier_reference_5_0_6.rs'
    expected_include = ('// Reuse the unchanged public-API observation driver.\n'
                        'include!("../../../rust_field_layout/src/bin/dynamic_carrier_5_0_6.rs");\n')
    if include.read_text() != expected_include:
        raise RuntimeError('Dynamic observer no longer includes the historical driver directly')
    for path in (original_driver, driver, include,
                 PARITY / 'rust_field_layout/src/bin/dynamic_carrier_5_0_6.rs',
                 PARITY / 'rust_values/src/fixture_value.rs',
                 PARITY / 'initialized_snapshot_cache_5_0_6.rs',
                 Path(__file__), reference / 'Cargo.lock'):
        record['observer_hashes'][str(path.relative_to(ROOT))] = digest(path)
    previous = read(args.implementation_review)
    if previous['runtime_commit'] != lock['commit']:
        raise RuntimeError('Implementation review belongs to another runtime baseline')
    record['implementation_review'] = dict(path=str(args.implementation_review),
                                            sha256=digest(args.implementation_review))
    for path, previous_hash in previous['implementation_hashes'].items():
        if not path.startswith(('teaql_core/', 'teaql_runtime/')):
            continue
        current = digest(ROOT / path)
        if current != previous_hash:
            raise RuntimeError(f'Previously qualified shared implementation changed: {path}')
        record['implementation_hashes'][path] = current
    for name, filename, rust_name, count in MATRICES:
        path = PARITY / filename
        corpus = read(path)
        if corpus['baseline'] != lock['commit'] or len(corpus['cases']) != count:
            raise RuntimeError(f'Corpus baseline/geometry changed: {filename}')
        inputs = [case['input'] for case in corpus['cases']]
        expected = [case['expected'] for case in corpus['cases']]
        rust = args.rust_dir / rust_name
        cpp = args.build_dir / (args.cpp_prefix + name + '_probe')
        original, actual = observe(str(rust), inputs), observe(str(cpp), inputs)
        entry = dict(name=name, count=count, corpus_sha256=digest(path),
                     rust_binary_sha256=digest(rust), cpp_binary_sha256=digest(cpp),
                     original=original, actual=actual,
                     saved_source_matches=same(expected, original),
                     current_source_matches=same(original, actual))
        record['matrices'].append(entry)
        save()
        if not entry['saved_source_matches'] or not entry['current_source_matches']:
            raise RuntimeError(f'Actual source/native observations differ: {name}')
        print(f'{name}: {count}/{count} exact source observations', flush=True)
    corpus_path = PARITY / 'initialized_snapshot_cache_5_0_6_cases.json.gz'
    corpus = read(corpus_path)
    inputs = list(cache_cases())
    if corpus['baseline'] != lock['commit'] or not same(inputs, [case['input'] for case in corpus['cases']]):
        raise RuntimeError('Initialized-cache baseline/inputs changed')
    original = observe(str(args.cache_rust), inputs)
    cpp = args.build_dir / (args.cpp_prefix + 'initialized_snapshot_cache_probe')
    actual = observe(str(cpp), inputs)
    cache = dict(name='initialized_snapshot_cache', count=80, ownership_adaptations=1,
                 corpus_sha256=digest(corpus_path), rust_binary_sha256=digest(args.cache_rust),
                 cpp_binary_sha256=digest(cpp), original=original, actual=actual)
    record['matrices'].append(cache)
    save()
    for input, saved, source, native in zip(inputs, corpus['cases'], original, actual):
        if not same(saved['source'].get('common'), source.get('common')) or not same(
                saved['source'].get('ownership'), source.get('ownership')):
            raise RuntimeError('Initialized-cache saved source semantics changed')
        if input['kind'] == 'ownership':
            validate_ownership(source, native)
        elif not same(source['common'], native['common']):
            raise RuntimeError('Initialized-cache source/native semantics differ')
        if input['kind'] != 'ownership' and (not source.get('diagnostics') or not native.get('diagnostics')):
            raise RuntimeError('Actual initialized diagnostics were omitted')
    record['cache_comparison'] = 'Exact stable semantics; actual diagnostics retained separately; explicit Arc/Context ownership adaptation'
    record['exact_semantic_observations'] = 44 + 427 + 176 + 80
    record['complete'] = True
    save()
    print('Initialized cache: 80/80 exact semantics; one explicit ownership adaptation', flush=True)


if __name__ == '__main__':
    main()
