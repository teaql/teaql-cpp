#!/usr/bin/env python3
"""Validate complete byte-preserved SQL executor driver source bindings."""
import argparse
import gzip
import hashlib
import json
import re
import tempfile
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from verify_constant_closure_5_0_6 import registry_identities



def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def check_dependency_lock(lock, reference_lock):
    assert registry_identities(lock) <= registry_identities(reference_lock)


def validate_sql_view(record, registry):
    assert len(registry['contracts']) == 9 and sum(c['count'] for c in registry['contracts']) == 8243
    view = record['sql_view']
    source = ROOT / '.reference/teaql-rs/teaql-sql'
    actual = {str(p.relative_to(source)): digest(p) for folder in ('src', 'tests', 'test-vectors')
              for p in (source / folder).rglob('*') if p.is_file()}
    assert view['all_original_bodies_preserved'] is True and view['source_files_sha256'] == actual
    text = (ROOT / registry['observer']).read_text()
    assert view['observer_text'] == text and view['observer_sha256'] == hashlib.sha256(text.encode()).hexdigest()
    assert record['copied_helpers'] == {p: digest(ROOT / p) for p in registry['helpers']}
    metadata = next(s for s in record['steps'] if s['command'][:2] == ['cargo', 'metadata']
                    and s['command'][-1] == str(source / 'Cargo.toml'))
    package = next(p for p in json.loads(metadata['stdout'])['packages'] if p['manifest_path'] == str(source / 'Cargo.toml'))
    assert package['name'] == 'teaql-sql' and package['version'] == '5.0.6' and package['edition'] == '2024'
    assert package['features'] == {}
    manifest = '[package]\nname="teaql-sql"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
    for kind, heading in [(None, 'dependencies'), ('dev', 'dev-dependencies')]:
        manifest += '\n[' + heading + ']\n'
        for dep in package['dependencies']:
            if dep['kind'] != kind:
                continue
            assert not dep.get('target')
            fields = ['version=' + json.dumps(dep['req'])]
            fields += [k + '=' + json.dumps(dep[k]) for k in ('path', 'features', 'optional') if dep.get(k)]
            if not dep.get('uses_default_features', True):
                fields.append('default-features=false')
            manifest += dep['name'] + '={' + ','.join(fields) + '}\n'
        if kind is None:
            manifest += 'serde_json="=1.0.150"\n'
    assert manifest == view['manifest']
    with tempfile.TemporaryDirectory(prefix='teaql_sql_driver_lock_contracts_506_') as temporary:
        lock = Path(temporary) / 'Cargo.lock'
        lock.write_text(view['lock'])
        check_dependency_lock(lock, ROOT / '.reference/teaql-rs/Cargo.lock')
        for project in record['projects'].values():
            lock.write_text(project['compiled_lock'])
            check_dependency_lock(lock, ROOT / '.reference/teaql-rs/Cargo.lock')
    expected = {'executor::tests': 16, 'executor::diagnostic_tests': 6,
                'executor::mutation_diagnostic_tests': 21, 'diagnostic_stream::tests': 7}
    assert len(record['original_sql_tests']) == 4
    assert {t['module']: t['count'] for t in record['original_sql_tests']} == expected
    for tests in record['original_sql_tests']:
        step = record['steps'][tests['step']]
        assert step['exit_code'] == 0 and '--offline' in step['command'] and '--locked' in step['command']
        assert step['command'][step['command'].index('--manifest-path') + 1] == str(Path(record['owned_directory']) / 'sql/Cargo.toml')
        names = re.findall(r'^test ([^ ]+) [.][.][.]', step['stdout'], re.M)
        assert names == tests['tests'] and len(names) == tests['count']
        assert all(n.startswith(tests['module'] + '::') for n in names)
        assert str(tests['count']) + ' passed; 0 failed' in step['stdout']


def validate(record, registry):
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert type(registry['contracts']) is list and registry['contracts']
    assert len({c['name'] for c in registry['contracts']}) == len(registry['contracts'])
    assert all(type(c['count']) is int and c['count'] > 0 for c in registry['contracts'])
    assert registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['input_hashes_before'] == record['input_hashes_after']
    assert record['steps'] and all(s['exit_code'] == 0 for s in record['steps'])
    reference = ROOT / '.reference/teaql-rs'
    names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    actual = {n.decode(): digest(reference / n.decode()) for n in names if n and (
        n.decode().endswith('.rs') or Path(n.decode()).name in ('Cargo.toml', 'Cargo.lock'))}
    assert actual == record['reference_source_files']
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_sql_executor_driver_replay_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment']['CARGO_TARGET_DIR'] == str(owned / 'target')
    assert record['cargo_environment']['CARGO_INCREMENTAL'] == record['cargo_environment']['CARGO_PROFILE_DEV_DEBUG'] == '0'
    assert digest(ROOT / 'scripts/replay_sql_executor_drivers_5_0_6.py') == record['input_hashes_after'][str(ROOT / 'scripts/replay_sql_executor_drivers_5_0_6.py')]
    assert set(record['projects']) == {c['project'] for c in registry['contracts']}
    validate_sql_view(record, registry)
    for name, details in record['projects'].items():
        manifest = ROOT / name
        assert details['manifest'] == manifest.read_text() and details['lock'] == manifest.with_name('Cargo.lock').read_text()
        check_dependency_lock(manifest.with_name('Cargo.lock'), reference / 'Cargo.lock')
        metadata = record['steps'][details['metadata_step']]
        build = record['steps'][details['build_step']]
        compiled_manifest = owned / 'project/Cargo.toml'
        def absolute_path(match):
            target = (manifest.parent / match[1]).resolve()
            if target == ROOT / '.reference/teaql-rs/teaql-sql':
                target = owned / 'sql'
            return 'path = ' + json.dumps(str(target))
        prepared = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, manifest.read_text())
        assert details['compiled_manifest'] == prepared
        assert metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version', '1', '--manifest-path', str(compiled_manifest)]
        assert build['command'][:2] == ['cargo', 'build'] and '--offline' in build['command'] and '--locked' in build['command']
        assert build['command'][build['command'].index('--manifest-path') + 1] == str(compiled_manifest)
        packages = json.loads(metadata['stdout'])['packages']
        package = next(p for p in packages if p['manifest_path'] == str(compiled_manifest))
        for p in packages:
            if p['source'] is None:
                assert Path(p['manifest_path']) == compiled_manifest or Path(p['manifest_path']).is_relative_to(reference) or Path(p['manifest_path']) == owned / 'sql/Cargo.toml'
        for c in registry['contracts']:
            if c['project'] == name:
                target = next(t for t in package['targets'] if t['name'] == c['target'])
                assert Path(target['src_path']).resolve() == (ROOT / c['driver']).resolve()
                assert any(build['command'][i:i + 2] == ['--bin', c['target']] for i in range(len(build['command']) - 1))
    assert len(record['contracts']) == len(registry['contracts'])
    assert {c['name'] for c in record['contracts']} == {c['name'] for c in registry['contracts']}
    results = {c['name']: c for c in record['contracts']}
    for c in registry['contracts']:
        corpus = read(ROOT / c['corpus'])
        assert digest(ROOT / c['corpus']) == c['corpus_sha256']
        assert corpus.get('driver', corpus.get('original_driver')) == c['driver']
        assert corpus.get('driver_sha256', corpus.get('original_driver_sha256')) == c['driver_sha256'] == digest(ROOT / c['driver'])
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        assert corpus['baseline'] == registry['runtime_commit'] and len(inputs) == c['count']
        if c['historical_corpus']:
            assert digest(ROOT / c['historical_corpus']) == c['historical_sha256'] == corpus['historical_corpus_sha256']
            assert same(inputs, [row['input'] for row in read(ROOT / c['historical_corpus'])['cases']])
        result = results[c['name']]
        assert result['count'] == c['count']
        payload = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in inputs)
        original = record['steps'][result['original_step']]
        native = record['steps'][result['native_step']]
        assert original['command'] == [str(owned / 'target/debug' / c['target'])]
        assert native['command'] == [str(ROOT / 'build_native_current' / c['native_target'])]
        for step in (original, native):
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            observed = json_lines(step['stdout'])
            assert len(observed) == c['count'] and same(observed, expected), c['name']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True, capture_output=True)
    registry_path, execution_path = args.contracts.resolve(), args.execution.resolve()
    registry, execution = read(registry_path), read(execution_path)
    validate(execution, registry)
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_provenance_complete=False,
            registry_sha256=digest(registry_path), execution_sha256=digest(execution_path), source_contracts=len(registry['contracts']),
            observations=sum(c['count'] for c in registry['contracts']), projects=len(execution['projects']), scope=__doc__)).encode(), mtime=0))
    print(len(registry['contracts']), 'complete source/corpus/native bindings verify without a Rust build; complete SQ02 stays open')


if __name__ == '__main__':
    main()
