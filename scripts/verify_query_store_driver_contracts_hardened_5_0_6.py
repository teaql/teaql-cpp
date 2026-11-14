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
from replay_query_store_drivers_hardened_5_0_6 import observer_source, extracted_sources
from verify_constant_closure_5_0_6 import registry_identities


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def expected_runtime_manifest(package):
    if not (package['name'] == 'teaql-runtime' and package['version'] == '5.0.6'):
        raise AssertionError
    if not (package['edition'] == '2024'):
        raise AssertionError
    if not (package['features'] == {'opentelemetry': ['dep:opentelemetry']}):
        raise AssertionError
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
            if not (not dep.get('target')):
                raise AssertionError
            fields = ['version=' + json.dumps(dep['req'])]
            fields += [key + '=' + json.dumps(dep[key]) for key in ['path', 'features', 'optional']
                       if dep.get(key)]
            if not dep.get('uses_default_features', True):
                fields.append('default-features=false')
            result += dep['name'] + '={' + ','.join(fields) + '}\n'
    if not (omitted == {'opentelemetry_sdk', 'opentelemetry-otlp'}):
        raise AssertionError
    return result + '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'


def validate(record, registry):
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError
    if not (record['registry'] == registry):
        raise AssertionError
    if not (len(registry['contracts']) == 3 and sum(c['count'] for c in registry['contracts']) == 477):
        raise AssertionError
    if not (registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']):
        raise AssertionError
    if not (registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']):
        raise AssertionError
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError
    if not (record['references_before'] == record['references_after']):
        raise AssertionError
    if not (record['references_after'] == {'teaql-rs': registry['runtime_commit'],
                                          'teaql_code_gen': registry['generator_commit']}):
        raise AssertionError
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_query_store_driver_replay_506_')):
        raise AssertionError
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError
    if not (record['cargo_environment'] == {'CARGO_INCREMENTAL': '0', 'CARGO_PROFILE_DEV_DEBUG': '0',
                                           'CARGO_TARGET_DIR': str(owned / 'target')}):
        raise AssertionError
    if not (record['observer_adapter_source'] == observer_source()):
        raise AssertionError
    if not (all(s['exit_code'] == 0 for s in record['steps'])):
        raise AssertionError
    replay = ROOT / 'scripts/replay_query_store_drivers_5_0_6.py'
    if not (record['input_hashes_after']['runtime:' + str(replay.relative_to(ROOT))] == digest(replay)):
        raise AssertionError
    project = ROOT / registry['project']
    if not (record['project_files'] == {name: (project.parent / name).read_text()
                                       for name in ['Cargo.toml', 'Cargo.lock', 'observers.json']}):
        raise AssertionError
    source_manifest = read(ROOT / registry['source_manifest'])
    if not (source_manifest['baseline'] == registry['runtime_commit']):
        raise AssertionError
    if not (all(digest(ROOT / path) == value for path, value in source_manifest['files'].items())):
        raise AssertionError
    reference = ROOT / '.reference/teaql-rs'
    actual_reference = {name: digest(reference / name) for name in record['reference_source_files']}
    if not (len(actual_reference) == 466 and record['reference_source_files'] == actual_reference):
        raise AssertionError
    if not (registry_identities(project.with_name('Cargo.lock')) <= registry_identities(reference / 'Cargo.lock')):
        raise AssertionError
    metadata_step = next(s for s in record['steps'] if s['label'] == 'metadata')
    if not (metadata_step['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                         '1', '--manifest-path', str(project)]):
        raise AssertionError
    if not (metadata_step['stdout'] == record['project_metadata']):
        raise AssertionError
    packages = json.loads(record['project_metadata'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    if not ({t['name'] for t in package['targets'] if 'bin' in t['kind']} == {
        c['target'] for c in registry['contracts']}):
        raise AssertionError
    if not (Path(next(t for t in package['targets'] if 'custom-build' in t['kind'])['src_path']).resolve() == (
        ROOT / registry['extractor']).resolve()):
        raise AssertionError
    for item in packages:
        if item['source'] is None:
            if not (Path(item['manifest_path']) == project or Path(item['manifest_path']).is_relative_to(reference)):
                raise AssertionError
    source = record['source_execution']
    if not (source['complete'] is True and source['full_goal_accepted'] is False):
        raise AssertionError
    if not (source['baseline'] == registry['runtime_commit']):
        raise AssertionError
    if not (source['verifier_sha256'] == hashlib.sha256(observer_source().encode()).hexdigest()):
        raise AssertionError
    if not (source['observer_project'] == record['project_files']):
        raise AssertionError
    if not (source['source_view_helper_sha256'] == digest(ROOT / 'scripts/reference_runtime_view_5_0_6.py')):
        raise AssertionError
    if not (len(source['matrices']) == 3 and source['cases_per_frontend'] == 477):
        raise AssertionError
    if not (all(s['exit_code'] == 0 for s in source['steps'])):
        raise AssertionError
    build = next(s for s in source['steps'] if s['command'][:2] == ['cargo', 'build'])
    if not ('--locked' in build['command'] and '--offline' in build['command'] and '--bins' in build['command']):
        raise AssertionError
    if not (build['command'][build['command'].index('--manifest-path') + 1] == str(project)):
        raise AssertionError
    expected_extractions = extracted_sources()
    if not (set(record['extracted_files']) == set(expected_extractions)):
        raise AssertionError
    for name, text in expected_extractions.items():
        if not (record['extracted_files'][name] == dict(text=text,
            sha256=hashlib.sha256(text.encode()).hexdigest())):
            raise AssertionError
    context_inputs, context_expected = [], []
    for contract, matrix in zip(registry['contracts'], source['matrices']):
        corpus = read(ROOT / contract['corpus'])
        if not (digest(ROOT / contract['corpus']) == contract['corpus_sha256']):
            raise AssertionError
        if not (digest(ROOT / contract['driver']) == contract['driver_sha256']):
            raise AssertionError
        if not (Path(next(t for t in package['targets'] if t['name'] == contract['target'])['src_path']).resolve() == (
            ROOT / contract['driver']).resolve()):
            raise AssertionError
        if not (corpus['baseline'] == registry['runtime_commit'] and len(corpus['cases']) == contract['count']):
            raise AssertionError
        inputs = [c['input'] for c in corpus['cases']]
        expected = [c['expected'] for c in corpus['cases']]
        if not (matrix['name'] == contract['name'] and matrix['cases'] == contract['count']):
            raise AssertionError
        if not (matrix['corpus_sha256'] == contract['corpus_sha256'] and same(matrix['original'], expected)):
            raise AssertionError
        if not (set(matrix['frontends']) == {'native'} and same(matrix['frontends']['native']['outcomes'], expected)):
            raise AssertionError
        payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
        for binary in [owned / 'target/debug' / contract['target'],
                       ROOT / 'build_native_current' / contract['native_target']]:
            step = next(s for s in source['steps'] if s['command'] == [str(binary)])
            if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
                raise AssertionError
            if not (step['stdin_bytes'] == len(payload.encode())):
                raise AssertionError
            if not (same(json_lines(step['stdout']), expected)):
                raise AssertionError
        if contract['name'] == 'query_store':
            vectors = [c for c in corpus['cases'] if c['input']['mode'] == 'observations']
            context_inputs = [c['input'] for c in vectors]
            context_expected = [c['expected'] for c in vectors]
    if not (len(context_inputs) == 36):
        raise AssertionError
    context = source['actual_context']
    files = source['actual_context_files']
    if not (context['contracts_passed'] == 2 and context['retained_vectors'] == 36):
        raise AssertionError
    if not (same(context['outputs'], context_expected)):
        raise AssertionError
    if not (same(json.loads(files['inputs']), context_inputs) and same(json.loads(files['outputs']), context_expected)):
        raise AssertionError
    for name in ['inputs', 'outputs']:
        if not (files[name + '_sha256'] == hashlib.sha256(files[name].encode()).hexdigest()):
            raise AssertionError
    view_dir = Path(files['directory'])
    if not (view_dir.is_relative_to(owned / 'children')):
        raise AssertionError
    if not (files['environment'] == {'TEAQL_STORE_CONTEXT_INPUTS': str(view_dir / 'inputs.json'),
                                    'TEAQL_STORE_CONTEXT_OUTPUTS': str(view_dir / 'outputs.json')}):
        raise AssertionError
    view = context['source_view']
    runtime = reference / 'teaql-runtime'
    original_files = {str(p.relative_to(runtime)): digest(p) for directory in ['src', 'tests', 'test-vectors']
                      for p in (runtime / directory).rglob('*') if p.is_file()}
    if not (view['source_files_sha256'] == original_files and view['all_original_bodies_preserved'] is True):
        raise AssertionError
    observer = ROOT / registry['context_observer']
    if not (view['observers'] == {'src/context/pagination.rs': {'text': observer.read_text(),
                                                             'sha256': digest(observer)}}):
        raise AssertionError
    if not (set(view['omitted_dev_dependencies']) == {'opentelemetry_sdk', 'opentelemetry-otlp'}):
        raise AssertionError
    runtime_meta = next(s for s in source['steps'] if s['command'][:2] == ['cargo', 'metadata']
                        and s['command'][-1] == str(runtime / 'Cargo.toml'))
    runtime_package = next(p for p in json.loads(runtime_meta['stdout'])['packages']
                           if p['manifest_path'] == str(runtime / 'Cargo.toml'))
    if not (view['manifest'] == expected_runtime_manifest(runtime_package)):
        raise AssertionError
    with tempfile.TemporaryDirectory(prefix='teaql_query_store_driver_lock_506_') as temporary:
        lock = Path(temporary) / 'Cargo.lock'
        lock.write_text(view['lock'])
        if not (registry_identities(lock) <= registry_identities(reference / 'Cargo.lock')):
            raise AssertionError
    tests = next(s for s in source['steps'] if s['command'][:2] == ['cargo', 'test'])
    if not ('--locked' in tests['command'] and '--offline' in tests['command']):
        raise AssertionError
    if not (tests['command'][tests['command'].index('--manifest-path') + 1] == str(view_dir / 'Cargo.toml')):
        raise AssertionError
    if not (set(re.findall(r'^test ([^ ]+) [.][.][.]', tests['stdout'], re.M)) == {
        'context::pagination::source_context_contracts_5_0_6::actual_context_observation_vectors',
        'context::pagination::source_context_contracts_5_0_6::actual_context_store_registration_and_lock_identity'}):
        raise AssertionError
    if not ('2 passed; 0 failed; 0 ignored' in tests['stdout']):
        raise AssertionError


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError
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
