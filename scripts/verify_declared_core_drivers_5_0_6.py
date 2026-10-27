#!/usr/bin/env python3
"""Validate declared core-driver compilation, dependencies and complete raw observations."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from verify_constant_closure_5_0_6 import registry_identities

REGISTRY = ROOT / 'tests/parity/declared_core_driver_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/declared_core_driver_execution_5_0_6.json.gz'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def check_dependency_lock(lock, reference_lock):
    if not (registry_identities(lock) <= registry_identities(reference_lock)):
        raise AssertionError()


def validate(record, registry):
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError()
    if not (record['registry'] == registry):
        raise AssertionError()
    if not (len(registry['contracts']) == 14 and sum(c['count'] for c in registry['contracts']) == 5656):
        raise AssertionError()
    if not (registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']):
        raise AssertionError()
    if not (registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']):
        raise AssertionError()
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError()
    if not (record['steps'] and all(s['exit_code'] == 0 for s in record['steps'])):
        raise AssertionError()
    reference = ROOT / '.reference/teaql-rs'
    names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    actual = {n.decode(): digest(reference / n.decode()) for n in names if n and (
        n.decode().endswith('.rs') or Path(n.decode()).name in ('Cargo.toml', 'Cargo.lock'))}
    if not (actual == record['reference_source_files']):
        raise AssertionError()
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_declared_core_drivers_506_')):
        raise AssertionError()
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError()
    if not (record['cargo_environment']['CARGO_TARGET_DIR'] == str(owned / 'target')):
        raise AssertionError()
    if not (record['cargo_environment']['CARGO_INCREMENTAL']
            == record['cargo_environment']['CARGO_PROFILE_DEV_DEBUG'] == '0'):
        raise AssertionError()
    if not (digest(ROOT / 'scripts/replay_declared_core_drivers_5_0_6.py')
            == record['input_hashes_after'][str(ROOT / 'scripts/replay_declared_core_drivers_5_0_6.py')]):
        raise AssertionError()
    if not (set(record['projects']) == {c['project'] for c in registry['contracts']}):
        raise AssertionError()
    for name, details in record['projects'].items():
        manifest = ROOT / name
        if not (details['manifest'] == manifest.read_text()
                and details['lock'] == manifest.with_name('Cargo.lock').read_text()):
            raise AssertionError()
        check_dependency_lock(manifest.with_name('Cargo.lock'), reference / 'Cargo.lock')
        if not ('teaql-runtime' not in details['manifest'] and 'teaql-sql' not in details['manifest']):
            raise AssertionError()
        metadata = record['steps'][details['metadata_step']]
        build = record['steps'][details['build_step']]
        if not (metadata['command'] == [
                'cargo', 'metadata', '--offline', '--locked', '--format-version', '1',
                '--manifest-path', str(manifest)]):
            raise AssertionError()
        if not (build['command'][:2] == ['cargo', 'build']
                and '--offline' in build['command'] and '--locked' in build['command']):
            raise AssertionError()
        if not (build['command'][build['command'].index('--manifest-path') + 1] == str(manifest)):
            raise AssertionError()
        packages = json.loads(metadata['stdout'])['packages']
        package = next(p for p in packages if p['manifest_path'] == str(manifest))
        for p in packages:
            if p['source'] is None:
                if not (Path(p['manifest_path']) == manifest or Path(p['manifest_path']).is_relative_to(reference)):
                    raise AssertionError()
        for c in registry['contracts']:
            if c['project'] == name:
                target = next(t for t in package['targets'] if t['name'] == c['target'])
                if not (Path(target['src_path']).resolve() == (ROOT / c['driver']).resolve()):
                    raise AssertionError()
                if not (any(build['command'][i:i + 2] == ['--bin', c['target']]
                            for i in range(len(build['command']) - 1))):
                    raise AssertionError()
    if not ({c['name'] for c in record['contracts']} == {c['name'] for c in registry['contracts']}):
        raise AssertionError()
    results = {c['name']: c for c in record['contracts']}
    for c in registry['contracts']:
        corpus = read(ROOT / c['corpus'])
        if not (digest(ROOT / c['corpus']) == c['corpus_sha256']):
            raise AssertionError()
        if not (corpus['driver'] == c['driver']):
            raise AssertionError()
        if not (corpus['driver_sha256'] == c['driver_sha256'] == digest(ROOT / c['driver'])):
            raise AssertionError()
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        if not (corpus['baseline'] == registry['runtime_commit'] and len(inputs) == c['count']):
            raise AssertionError()
        if c['historical_corpus']:
            if not (digest(ROOT / c['historical_corpus']) == c['historical_sha256']
                    == corpus['historical_corpus_sha256']):
                raise AssertionError()
            if not (same(inputs, [row['input'] for row in read(ROOT / c['historical_corpus'])['cases']])):
                raise AssertionError()
        result = results[c['name']]
        if not (result['count'] == c['count']):
            raise AssertionError()
        payload = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in inputs)
        original = record['steps'][result['original_step']]
        native = record['steps'][result['native_step']]
        if not (original['command'] == [str(owned / 'target/debug' / c['target'])]):
            raise AssertionError()
        if not (native['command'] == [str(ROOT / 'build_native_current' / c['native_target'])]):
            raise AssertionError()
        for step in (original, native):
            if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
                raise AssertionError()
            if not (step['stdin_bytes'] == len(payload.encode())):
                raise AssertionError()
            observed = json_lines(step['stdout'])
            if not (len(observed) == c['count'] and same(observed, expected)):
                raise AssertionError(c['name'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True, capture_output=True)
    registry, execution = read(REGISTRY), read(EXECUTION)
    validate(execution, registry)
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_provenance_complete=False,
            registry_sha256=digest(REGISTRY), execution_sha256=digest(EXECUTION), source_contracts=14,
            observations=5656, projects=6, scope=__doc__)).encode(), mtime=0))
    print('Fourteen complete source/corpus/native bindings verify without a Rust build; complete SQ02 stays open')


if __name__ == '__main__':
    main()
