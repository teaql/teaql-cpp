#!/usr/bin/env python3
"""Verify retained query-store extraction and full-Context provenance without Rust builds."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from replay_query_store_drivers_5_0_6 import observer_source, extracted_sources
from verify_constant_closure_5_0_6 import registry_identities


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def expected_runtime_manifest(package):
    assert package['name'] == 'teaql-runtime' and package['version'] == '5.0.6'
    assert package['edition'] == '2024'
    assert package['features'] == {'opentelemetry': ['dep:opentelemetry']}
    result = '[package]\nname="teaql-runtime"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
    omitted = set()
    for kind, heading in [(None, 'dependencies'), ('dev', 'dev-dependencies')]:
        result += '\n[' + heading + ']\n'
        for dep in package['dependencies']:
            if dep['kind'] != kind:
                continue
            if kind == 'dev' and dep['name'] != 'teaql-macros':
                omitted.add(dep['name'])
                continue
            assert not dep.get('target')
            fields = ['version=' + json.dumps(dep['req'])]
            fields += [key + '=' + json.dumps(dep[key]) for key in ['path', 'features', 'optional']
                       if dep.get(key)]
            if not dep.get('uses_default_features', True):
                fields.append('default-features=false')
            result += dep['name'] + '={' + ','.join(fields) + '}\n'
    assert omitted == {'opentelemetry_sdk', 'opentelemetry-otlp'}
    return result + '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'


def validate(record, registry):
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert len(registry['contracts']) == 3 and sum(c['count'] for c in registry['contracts']) == 477
    assert registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['input_hashes_before'] == record['input_hashes_after']
    assert record['references_before'] == record['references_after']
    assert record['references_after'] == {'teaql-rs': registry['runtime_commit'],
                                          'teaql_code_gen': registry['generator_commit']}
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_query_store_driver_replay_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == {'CARGO_INCREMENTAL': '0', 'CARGO_PROFILE_DEV_DEBUG': '0',
                                           'CARGO_TARGET_DIR': str(owned / 'target')}
    assert record['observer_adapter_source'] == observer_source()
    assert all(s['exit_code'] == 0 for s in record['steps'])
    replay = ROOT / 'scripts/replay_query_store_drivers_5_0_6.py'
    assert record['input_hashes_after']['runtime:' + str(replay.relative_to(ROOT))] == digest(replay)
    project = ROOT / registry['project']
    assert record['project_files'] == {name: (project.parent / name).read_text()
                                       for name in ['Cargo.toml', 'Cargo.lock', 'observers.json']}
    source_manifest = read(ROOT / registry['source_manifest'])
    assert source_manifest['baseline'] == registry['runtime_commit']
    assert all(digest(ROOT / path) == value for path, value in source_manifest['files'].items())
    reference = ROOT / '.reference/teaql-rs'
    actual_reference = {name: digest(reference / name) for name in record['reference_source_files']}
    assert len(actual_reference) == 466 and record['reference_source_files'] == actual_reference
    assert registry_identities(project.with_name('Cargo.lock')) <= registry_identities(reference / 'Cargo.lock')
    metadata_step = next(s for s in record['steps'] if s['label'] == 'metadata')
    assert metadata_step['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                         '1', '--manifest-path', str(project)]
    assert metadata_step['stdout'] == record['project_metadata']
    packages = json.loads(record['project_metadata'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    assert {t['name'] for t in package['targets'] if 'bin' in t['kind']} == {
        c['target'] for c in registry['contracts']}
    assert Path(next(t for t in package['targets'] if 'custom-build' in t['kind'])['src_path']).resolve() == (
        ROOT / registry['extractor']).resolve()
    for item in packages:
        if item['source'] is None:
            assert Path(item['manifest_path']) == project or Path(item['manifest_path']).is_relative_to(reference)
    source = record['source_execution']
    assert source['complete'] is True and source['full_goal_accepted'] is False
    assert source['baseline'] == registry['runtime_commit']
    assert source['verifier_sha256'] == hashlib.sha256(observer_source().encode()).hexdigest()
    assert source['observer_project'] == record['project_files']
    assert source['source_view_helper_sha256'] == digest(ROOT / 'scripts/reference_runtime_view_5_0_6.py')
    assert len(source['matrices']) == 3 and source['cases_per_frontend'] == 477
    assert all(s['exit_code'] == 0 for s in source['steps'])
    build = next(s for s in source['steps'] if s['command'][:2] == ['cargo', 'build'])
    assert '--locked' in build['command'] and '--offline' in build['command'] and '--bins' in build['command']
    assert build['command'][build['command'].index('--manifest-path') + 1] == str(project)
    expected_extractions = extracted_sources()
    assert set(record['extracted_files']) == set(expected_extractions)
    for name, text in expected_extractions.items():
        assert record['extracted_files'][name] == dict(text=text,
            sha256=hashlib.sha256(text.encode()).hexdigest())
    context_inputs, context_expected = [], []
    for contract, matrix in zip(registry['contracts'], source['matrices']):
        corpus = read(ROOT / contract['corpus'])
        assert digest(ROOT / contract['corpus']) == contract['corpus_sha256']
        assert digest(ROOT / contract['driver']) == contract['driver_sha256']
        assert Path(next(t for t in package['targets'] if t['name'] == contract['target'])['src_path']).resolve() == (
            ROOT / contract['driver']).resolve()
        assert corpus['baseline'] == registry['runtime_commit'] and len(corpus['cases']) == contract['count']
        inputs = [c['input'] for c in corpus['cases']]
        expected = [c['expected'] for c in corpus['cases']]
        assert matrix['name'] == contract['name'] and matrix['cases'] == contract['count']
        assert matrix['corpus_sha256'] == contract['corpus_sha256'] and same(matrix['original'], expected)
        assert set(matrix['frontends']) == {'native'} and same(matrix['frontends']['native']['outcomes'], expected)
        payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
        for binary in [owned / 'target/debug' / contract['target'],
                       ROOT / 'build_native_current' / contract['native_target']]:
            step = next(s for s in source['steps'] if s['command'] == [str(binary)])
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            assert same(json_lines(step['stdout']), expected)
        if contract['name'] == 'query_store':
            vectors = [c for c in corpus['cases'] if c['input']['mode'] == 'observations']
            context_inputs = [c['input'] for c in vectors]
            context_expected = [c['expected'] for c in vectors]
    assert len(context_inputs) == 36
    context = source['actual_context']
    files = source['actual_context_files']
    assert context['contracts_passed'] == 2 and context['retained_vectors'] == 36
    assert same(context['outputs'], context_expected)
    assert same(json.loads(files['inputs']), context_inputs) and same(json.loads(files['outputs']), context_expected)
    for name in ['inputs', 'outputs']:
        assert files[name + '_sha256'] == hashlib.sha256(files[name].encode()).hexdigest()
    view_dir = Path(files['directory'])
    assert view_dir.is_relative_to(owned / 'children')
    assert files['environment'] == {'TEAQL_STORE_CONTEXT_INPUTS': str(view_dir / 'inputs.json'),
                                    'TEAQL_STORE_CONTEXT_OUTPUTS': str(view_dir / 'outputs.json')}
    view = context['source_view']
    runtime = reference / 'teaql-runtime'
    original_files = {str(p.relative_to(runtime)): digest(p) for directory in ['src', 'tests', 'test-vectors']
                      for p in (runtime / directory).rglob('*') if p.is_file()}
    assert view['source_files_sha256'] == original_files and view['all_original_bodies_preserved'] is True
    observer = ROOT / registry['context_observer']
    assert view['observers'] == {'src/context/pagination.rs': {'text': observer.read_text(),
                                                             'sha256': digest(observer)}}
    assert set(view['omitted_dev_dependencies']) == {'opentelemetry_sdk', 'opentelemetry-otlp'}
    runtime_meta = next(s for s in source['steps'] if s['command'][:2] == ['cargo', 'metadata']
                        and s['command'][-1] == str(runtime / 'Cargo.toml'))
    runtime_package = next(p for p in json.loads(runtime_meta['stdout'])['packages']
                           if p['manifest_path'] == str(runtime / 'Cargo.toml'))
    assert view['manifest'] == expected_runtime_manifest(runtime_package)
    with tempfile.TemporaryDirectory(prefix='teaql_query_store_driver_lock_506_') as temporary:
        lock = Path(temporary) / 'Cargo.lock'
        lock.write_text(view['lock'])
        assert registry_identities(lock) <= registry_identities(reference / 'Cargo.lock')
    tests = next(s for s in source['steps'] if s['command'][:2] == ['cargo', 'test'])
    assert '--locked' in tests['command'] and '--offline' in tests['command']
    assert tests['command'][tests['command'].index('--manifest-path') + 1] == str(view_dir / 'Cargo.toml')
    assert set(re.findall(r'^test ([^ ]+) [.][.][.]', tests['stdout'], re.M)) == {
        'context::pagination::source_context_contracts_5_0_6::actual_context_observation_vectors',
        'context::pagination::source_context_contracts_5_0_6::actual_context_store_registration_and_lock_identity'}
    assert '2 passed; 0 failed; 0 ignored' in tests['stdout']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    record, registry = read(args.execution), read(args.contracts)
    validate(record, registry)
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True,
            whole_provenance_complete=False, source_contracts=3, observations=477,
            actual_context_subset=36, appended_context_tests=2,
            registry_sha256=digest(args.contracts), execution_sha256=digest(args.execution))).encode(), mtime=0))
    print('Three complete store/lock/model bindings and actual Context supplement verify without Rust builds.')


if __name__ == '__main__':
    main()
