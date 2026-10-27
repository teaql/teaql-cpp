#!/usr/bin/env python3
"""Verify the complete fresh source record without rerunning its Rust builds."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from corpus_input_contracts_5_0_6 import generate
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
from verify_corpus_source_contracts_5_0_6 import expected_manifest

REGISTRY = ROOT / 'tests/parity/remaining_corpus_source_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/remaining_corpus_source_execution_5_0_6.json.gz'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def validate_common(record, registry, directory):
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError()
    if not (record['registry'] == registry):
        raise AssertionError()
    if not (len(registry['contracts']) == 7 and sum(c['count'] for c in registry['contracts']) == 6281):
        raise AssertionError()
    if not (registry['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']):
        raise AssertionError()
    if not (registry['generator_commit'] == json.loads(
            (ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']):
        raise AssertionError()
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError()
    if not (record['steps'] and all(s['exit_code'] == 0 for s in record['steps'])):
        raise AssertionError()
    reference = ROOT / '.reference/teaql-rs'
    names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    actual_sources = {name.decode(): digest(reference / name.decode()) for name in names if name and (
        name.decode().endswith('.rs') or Path(name.decode()).name in ('Cargo.toml', 'Cargo.lock'))}
    if not (record['reference_source_files'] == actual_sources):
        raise AssertionError()
    source_inputs = {REGISTRY, ROOT / 'upstream.lock.json', ROOT / 'scripts/corpus_input_contracts_5_0_6.py',
                     ROOT / 'scripts/replay_remaining_corpus_sources_5_0_6.py',
                     ROOT / 'scripts/reference_runtime_view_5_0_6.py', ROOT / 'tests/parity/compare_values.py',
                     ROOT / 'tests/parity/query_full_state_5_0_6.rs',
                     ROOT / 'tests/parity/rust_values/src/fixture_value.rs'}
    source_inputs.update(ROOT / c['source'] for c in registry['contracts'])
    source_inputs.update(ROOT / c['input_builder']['path'] for c in registry['contracts'])
    source_inputs.update(ROOT / c['corpus'] for c in registry['contracts'])
    projects = {c['project'] for c in registry['contracts'] if c['project']}
    source_inputs.update(ROOT / p for project in projects
                         for p in (project, str(Path(project).with_name('Cargo.lock'))))
    if not (all(record['input_hashes_after'][str(p)] == digest(p) for p in source_inputs)):
        raise AssertionError()
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_remaining_corpus_source_506_')):
        raise AssertionError()
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError()
    if not (record['cargo_environment']['CARGO_TARGET_DIR'] == str(owned / 'target')):
        raise AssertionError()
    if not (record['cargo_environment']['CARGO_INCREMENTAL'] == '0'):
        raise AssertionError()
    if not (record['cargo_environment']['CARGO_PROFILE_DEV_DEBUG']
            == record['cargo_environment']['CARGO_PROFILE_TEST_DEBUG'] == '0'):
        raise AssertionError()
    if not (all(s['command'][0] != 'cargo' or s['command'][1] not in ('test', 'build')
               or '--offline' in s['command'] and '--locked' in s['command'] for s in record['steps'])):
        raise AssertionError()
    view = record['runtime_view']
    if not (view['all_original_bodies_preserved'] is True):
        raise AssertionError()
    if not (set(view['omitted_dev_dependencies']) == {'opentelemetry_sdk', 'opentelemetry-otlp'}):
        raise AssertionError()
    source = reference / 'teaql-runtime'
    hashes = {str(p.relative_to(source)): digest(p) for folder in ('src', 'tests', 'test-vectors')
              for p in (source / folder).rglob('*') if p.is_file()}
    if not (view['source_files_sha256'] == hashes):
        raise AssertionError()
    if not (set(view['observers']) == {'src/generated_support.rs', 'src/data_service/mod.rs'}):
        raise AssertionError()
    support = '#[cfg(test)]\nmod generated_support_source_contracts_506 {\n' + (
        ROOT / 'tests/parity/query_full_state_5_0_6.rs').read_text() + '\n' + (
        ROOT / 'tests/parity/generated_support_source_5_0_6.rs').read_text() + '\n}\n'
    privacy = (ROOT / 'tests/parity/mutation_intent_privacy_source_5_0_6.rs').read_text().replace(
        '__TEAQL_INTENT_PRIVACY_INPUTS__', str(owned / 'inputs.json'))
    for name, text in [('src/generated_support.rs', support), ('src/data_service/mod.rs', privacy)]:
        if not (view['observers'][name] == dict(text=text, sha256=hashlib.sha256(text.encode()).hexdigest())):
            raise AssertionError()
    metadata = [s for s in record['steps'] if s['command'][:2] == ['cargo', 'metadata']
                and s['command'][-1] == str(source / 'Cargo.toml')]
    if not (len(metadata) == 1):
        raise AssertionError()
    package = next(p for p in json.loads(metadata[0]['stdout'])['packages']
                   if p['manifest_path'] == str(source / 'Cargo.toml'))
    if not (view['manifest'] == expected_manifest(package)):
        raise AssertionError()
    lock = directory / 'view.lock'
    lock.write_text(view['lock'])
    reference_lock = reference / 'Cargo.lock'
    if not (registry_identities(lock) <= registry_identities(reference_lock)):
        raise AssertionError()
    known = versions(reference_lock)
    if not (all(v <= known.get(k, set()) for k, v in versions(lock).items())):
        raise AssertionError()
    if not (set(record['projects']) == projects):
        raise AssertionError()
    for project, details in record['projects'].items():
        manifest = ROOT / project
        if not (details['manifest'] == manifest.read_text()):
            raise AssertionError()
        if not (details['lock'] == manifest.with_name('Cargo.lock').read_text()):
            raise AssertionError()
        if not (registry_identities(manifest.with_name('Cargo.lock')) <= registry_identities(reference_lock)):
            raise AssertionError()
        metadata = record['steps'][details['metadata_step']]
        build = record['steps'][details['build_step']]
        if not (metadata['command'] == [
                'cargo', 'metadata', '--offline', '--locked', '--format-version', '1',
                '--manifest-path', str(manifest)]):
            raise AssertionError()
        if not (build['command'][:2] == ['cargo', 'build']):
            raise AssertionError()
        if not (build['command'][build['command'].index('--manifest-path') + 1] == str(manifest)):
            raise AssertionError()
        packages = json.loads(metadata['stdout'])['packages']
        root_package = next(p for p in packages if p['manifest_path'] == str(manifest))
        if not (root_package['edition'] == '2024'):
            raise AssertionError()
        for p in packages:
            if p['source'] is None:
                path = Path(p['manifest_path'])
                if not (path == manifest or path.is_relative_to(reference)):
                    raise AssertionError()
        for c in registry['contracts']:
            if c['project'] != project:
                continue
            target = next(t for t in root_package['targets'] if t['name'] == c['reference_target'])
            if not (Path(target['src_path']).resolve() == (ROOT / c['source']).resolve()):
                raise AssertionError()
            flag = '--example' if c['source_kind'] == 'example' else '--bin'
            if not (any(build['command'][i:i + 2] == [flag, c['reference_target']]
                        for i in range(len(build['command']) - 1))):
                raise AssertionError()
    return dict(original_reference_files=len(record['reference_source_files']), runtime_files=len(hashes),
                private_registry_packages=len(registry_identities(lock)), projects=len(projects))


def validate_contract(contract, result, record):
    if not (result['name'] == contract['name'] and result['count'] == contract['count']):
        raise AssertionError()
    corpus_path = ROOT / contract['corpus']
    corpus = read(corpus_path)
    if not (corpus['baseline'] == record['registry']['runtime_commit']):
        raise AssertionError()
    if not (result['corpus_sha256'] == digest(corpus_path)):
        raise AssertionError()
    inputs, proof = generate(contract)
    if not (result['input_proof'] == proof and same(inputs, [c['input'] for c in corpus['cases']])):
        raise AssertionError()
    if not (digest(ROOT / contract['source']) == contract['source_sha256']):
        raise AssertionError()
    original = result['reference']
    step = record['steps'][original['step']]
    if not (step['exit_code'] == 0):
        raise AssertionError()
    if original['output_kind'] == 'file':
        if not (contract['source_kind'] == 'view_support'):
            raise AssertionError()
        if not (hashlib.sha256(original['raw_output'].encode()).hexdigest() == original['output_sha256']):
            raise AssertionError()
        observations = json.loads(original['raw_output'])
        if not (set(original['tests']) == {
                'generated_support::generated_support_source_contracts_506::observe_private_sources',
                'generated_support::generated_support_source_contracts_506::source_interface_contracts'}):
            raise AssertionError()
        if not ('2 passed; 0 failed' in step['stdout'] and 'Compiling teaql-runtime v5.0.6' in step['stderr']):
            raise AssertionError()
    elif original['output_kind'] == 'marker_stdout':
        if not (contract['source_kind'] == 'view_privacy'):
            raise AssertionError()
        if not (original['prefix'] == 'TEAQL_INTENT_PRIVACY_JSON '):
            raise AssertionError()
        observations = [json.loads(line.split(original['prefix'], 1)[1]) for line in step['stdout'].split('\n')
                        if original['prefix'] in line]
        if not (original['tests'] == [
                'data_service::cpp_mutation_intent_privacy_observer::replacing_intent_preserves_existing_privacy']):
            raise AssertionError()
        if not ('1 passed; 0 failed' in step['stdout']):
            raise AssertionError()
    else:
        if not (original['output_kind'] == 'stdout' and contract['source_kind'] in ('crate', 'example')):
            raise AssertionError()
        target = Path(record['owned_directory']) / 'target/debug'
        if contract['source_kind'] == 'example':
            target /= 'examples'
        if not (step['command'] == [str(target / contract['reference_target'])]):
            raise AssertionError()
        observations = json_lines(step['stdout'])
    if original['output_kind'] != 'stdout':
        if not (step['command'][:2] == ['cargo', 'test']):
            raise AssertionError()
        if not (step['command'][step['command'].index('--manifest-path') + 1]
                == str(Path(record['owned_directory']) / 'runtime/Cargo.toml')):
            raise AssertionError()
        actual_tests = re.findall(r'^test (\S+) \.\.\.', step['stdout'], re.M)
        if not (set(actual_tests) == set(original['tests'])):
            raise AssertionError()
    payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
    if original['output_kind'] == 'stdout':
        if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
            raise AssertionError()
        if not (step['stdin_bytes'] == len(payload.encode())):
            raise AssertionError()
    if not (type(observations) is list and len(observations) == contract['count']):
        raise AssertionError()
    if not (same(observations, original['observations'])):
        raise AssertionError()
    if not (same(observations, [c['expected'] for c in corpus['cases']])):
        raise AssertionError()
    native = record['steps'][result['native_step']]
    if not (native['exit_code'] == 0):
        raise AssertionError()
    if not (native['command'] == [str(ROOT / 'build_native_current' / contract['native_target'])]):
        raise AssertionError()
    if not (native['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
        raise AssertionError()
    if not (native['stdin_bytes'] == len(payload.encode())):
        raise AssertionError()
    if not (same(json_lines(native['stdout']), observations)):
        raise AssertionError()
    return dict(name=contract['name'], count=len(observations), source_kind=contract['source_kind'],
                source_step=original['step'], native_step=result['native_step'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError()
    registry = json.loads(REGISTRY.read_text())
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True, capture_output=True)
    execution = read(EXECUTION)
    record = dict(complete=False, scope=__doc__, contracts=[], execution_sha256=digest(EXECUTION))
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_remaining_corpus_contracts_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            record['source_validation'] = validate_common(execution, registry, owned)
            if not ([c['name'] for c in registry['contracts']] == [c['name'] for c in execution['contracts']]):
                raise AssertionError()
            for c, result in zip(registry['contracts'], execution['contracts']):
                record['contracts'].append(validate_contract(c, result, execution))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Seven complete fresh source/corpus/native bindings verified without a Rust build; full SQ02 remains open')


if __name__ == '__main__':
    main()
