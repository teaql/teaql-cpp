#!/usr/bin/env python3
"""Validate complete byte-preserved entity-runtime driver source bindings."""
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
    if not registry_identities(lock) <= registry_identities(reference_lock):
        raise AssertionError()



def check_corpus_binding(corpus, contract):
    if not digest(ROOT / contract['driver']) == contract['driver_sha256']:
        raise AssertionError()
    kind = contract['binding_kind']
    declared = corpus.get('driver', corpus.get('original_driver'))
    declared_hash = corpus.get('driver_sha256', corpus.get('original_driver_sha256'))
    if not (declared == contract['declared_driver'] and declared_hash == contract['declared_driver_sha256']):
        raise AssertionError()
    if kind == 'root_adapter':
        if not contract['name'] == 'entity_runtime':
            raise AssertionError()
        if not digest(ROOT / declared) == declared_hash:
            raise AssertionError()
        marker = 'use pinned::*;\n'
        original = (ROOT / declared).read_text()
        compiled = (ROOT / contract['driver']).read_text()
        if not original.count(marker) == compiled.count(marker) == 1:
            raise AssertionError()
        if not original.split(marker, 1)[1] == compiled.split(marker, 1)[1]:
            raise AssertionError()
    elif kind == 'source_hashes':
        if not (contract['name'] == 'dynamic_ledger' and declared is None and (declared_hash is None)):
            raise AssertionError()
        if not set(corpus['source_sha256']) == {'teaql-runtime/src/entity_runtime.rs', 'teaql-core/src/dynamic_fields.rs'}:
            raise AssertionError()
        if not all((digest(ROOT / '.reference/teaql-rs' / p) == h for (p, h) in corpus['source_sha256'].items())):
            raise AssertionError()
    else:
        if not (kind == 'declared' and declared == contract['driver']):
            raise AssertionError()
        if not declared_hash == contract['driver_sha256']:
            raise AssertionError()


def original_test_names():
    text = (ROOT / '.reference/teaql-rs/teaql-runtime/src/entity_runtime.rs').read_text()
    modules = list(re.finditer(r'^mod (\w+) \{', text, re.M))
    names = []
    for index, module in enumerate(modules):
        end = modules[index + 1].start() if index + 1 < len(modules) else len(text)
        names += ['entity_runtime::' + module[1] + '::' + name
                  for name in re.findall(r'#\[test\]\s*fn (\w+)\(', text[module.end():end])]
    if not len(names) == len(set(names)) == len(re.findall('#\\[test\\]', text)) == 21:
        raise AssertionError()
    return sorted(names)


def validate_runtime_view(record, registry):
    if not (len(registry['contracts']) == 7 and sum((c['count'] for c in registry['contracts'])) == 4226):
        raise AssertionError()
    view = record['runtime_view']
    source = ROOT / '.reference/teaql-rs/teaql-runtime'
    actual = {str(p.relative_to(source)): digest(p) for folder in ('src', 'tests', 'test-vectors')
              for p in (source / folder).rglob('*') if p.is_file()}
    if not (view['all_original_bodies_preserved'] is True and view['source_files_sha256'] == actual):
        raise AssertionError()
    if not record['observer_inputs'] == {p: digest(ROOT / p) for p in registry['observers']}:
        raise AssertionError()
    appendix = b'\n'.join((ROOT / p).read_bytes() for p in registry['observers'])
    if not set(view['observers']) == {'src/entity_runtime.rs'}:
        raise AssertionError()
    observer = view['observers']['src/entity_runtime.rs']
    if not observer['text'].encode() == appendix:
        raise AssertionError()
    if not observer['sha256'] == hashlib.sha256(appendix).hexdigest():
        raise AssertionError()
    metadata = next(s for s in record['steps'] if s['command'][:2] == ['cargo', 'metadata']
                    and s['command'][-1] == str(source / 'Cargo.toml'))
    package = next(p for p in json.loads(metadata['stdout'])['packages']
                   if p['manifest_path'] == str(source / 'Cargo.toml'))
    if not (package['name'] == 'teaql-runtime' and package['version'] == '5.0.6' and (package['edition'] == '2024')):
        raise AssertionError()
    if not package['features'] == {'opentelemetry': ['dep:opentelemetry']}:
        raise AssertionError()
    omitted = {'opentelemetry_sdk', 'opentelemetry-otlp'}
    if not set(view['omitted_dev_dependencies']) == omitted:
        raise AssertionError()
    manifest = '[package]\nname="teaql-runtime"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
    seen_omitted = set()
    for kind, heading in [(None, 'dependencies'), ('dev', 'dev-dependencies')]:
        manifest += '\n[' + heading + ']\n'
        for dep in package['dependencies']:
            if dep['kind'] != kind:
                continue
            if kind == 'dev' and dep['name'] != 'teaql-macros':
                if not dep['name'] in omitted:
                    raise AssertionError()
                seen_omitted.add(dep['name'])
                continue
            if not not dep.get('target'):
                raise AssertionError()
            fields = ['version=' + json.dumps(dep['req'])]
            fields += [k + '=' + json.dumps(dep[k]) for k in ('path', 'features', 'optional') if dep.get(k)]
            if not dep.get('uses_default_features', True):
                fields.append('default-features=false')
            manifest += dep['name'] + '={' + ','.join(fields) + '}\n'
    manifest += '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'
    if not (seen_omitted == omitted and manifest == view['manifest']):
        raise AssertionError()
    with tempfile.TemporaryDirectory(prefix='teaql_entity_runtime_driver_locks_506_') as temporary:
        lock = Path(temporary) / 'Cargo.lock'
        lock.write_text(view['lock'])
        check_dependency_lock(lock, ROOT / '.reference/teaql-rs/Cargo.lock')
        for project in record['projects'].values():
            lock.write_text(project['compiled_lock'])
            check_dependency_lock(lock, ROOT / '.reference/teaql-rs/Cargo.lock')
    tests = record['original_entity_runtime_tests']
    step = record['steps'][tests['step']]
    if not (step['exit_code'] == 0 and '--offline' in step['command'] and ('--locked' in step['command'])):
        raise AssertionError()
    if not step['command'][step['command'].index('--manifest-path') + 1] == str(Path(record['owned_directory']) / 'runtime/Cargo.toml'):
        raise AssertionError()
    names = re.findall(r'^test ([^ ]+) [.][.][.]', step['stdout'], re.M)
    if not (names == tests['tests'] and len(names) == tests['count'] == 21):
        raise AssertionError()
    if not sorted(names) == original_test_names():
        raise AssertionError()
    if not '21 passed; 0 failed; 0 ignored' in step['stdout']:
        raise AssertionError()


def validate(record, registry):
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError()
    if not record['registry'] == registry:
        raise AssertionError()
    if not (type(registry['contracts']) is list and registry['contracts']):
        raise AssertionError()
    if not len({c['name'] for c in registry['contracts']}) == len(registry['contracts']):
        raise AssertionError()
    if not all((type(c['count']) is int and c['count'] > 0 for c in registry['contracts'])):
        raise AssertionError()
    if not registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']:
        raise AssertionError()
    if not registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']:
        raise AssertionError()
    if not record['input_hashes_before'] == record['input_hashes_after']:
        raise AssertionError()
    if not (record['steps'] and all((s['exit_code'] == 0 for s in record['steps']))):
        raise AssertionError()
    reference = ROOT / '.reference/teaql-rs'
    names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    actual = {n.decode(): digest(reference / n.decode()) for n in names if n and (
        n.decode().endswith('.rs') or Path(n.decode()).name in ('Cargo.toml', 'Cargo.lock'))}
    if not actual == record['reference_source_files']:
        raise AssertionError()
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_entity_runtime_driver_replay_506_')):
        raise AssertionError()
    if not (record['owned_directory_removed'] is True and (not owned.exists())):
        raise AssertionError()
    if not record['cargo_environment']['CARGO_TARGET_DIR'] == str(owned / 'target'):
        raise AssertionError()
    if not record['cargo_environment']['CARGO_INCREMENTAL'] == record['cargo_environment']['CARGO_PROFILE_DEV_DEBUG'] == '0':
        raise AssertionError()
    if not digest(ROOT / 'scripts/replay_entity_runtime_drivers_5_0_6.py') == record['input_hashes_after'][str(ROOT / 'scripts/replay_entity_runtime_drivers_5_0_6.py')]:
        raise AssertionError()
    if not set(record['projects']) == {c['project'] for c in registry['contracts']}:
        raise AssertionError()
    validate_runtime_view(record, registry)
    for name, details in record['projects'].items():
        manifest = ROOT / name
        if not (details['manifest'] == manifest.read_text() and details['lock'] == manifest.with_name('Cargo.lock').read_text()):
            raise AssertionError()
        check_dependency_lock(manifest.with_name('Cargo.lock'), reference / 'Cargo.lock')
        metadata = record['steps'][details['metadata_step']]
        build = record['steps'][details['build_step']]
        compiled_manifest = owned / 'project/Cargo.toml'
        def absolute_path(match):
            target = (manifest.parent / match[1]).resolve()
            if target == ROOT / '.reference/teaql-rs/teaql-runtime':
                target = owned / 'runtime'
            return 'path = ' + json.dumps(str(target))
        prepared = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, manifest.read_text())
        if not details['compiled_manifest'] == prepared:
            raise AssertionError()
        if not metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version', '1', '--manifest-path', str(compiled_manifest)]:
            raise AssertionError()
        if not (build['command'][:2] == ['cargo', 'build'] and '--offline' in build['command'] and ('--locked' in build['command'])):
            raise AssertionError()
        if not build['command'][build['command'].index('--manifest-path') + 1] == str(compiled_manifest):
            raise AssertionError()
        packages = json.loads(metadata['stdout'])['packages']
        package = next(p for p in packages if p['manifest_path'] == str(compiled_manifest))
        if not {t['name'] for t in package['targets'] if 'bin' in t['kind']} == {c['target'] for c in registry['contracts'] if c['project'] == name}:
            raise AssertionError()
        for p in packages:
            if p['source'] is None:
                if not (Path(p['manifest_path']) == compiled_manifest or Path(p['manifest_path']).is_relative_to(reference) or Path(p['manifest_path']) == owned / 'runtime/Cargo.toml'):
                    raise AssertionError()
        for c in registry['contracts']:
            if c['project'] == name:
                target = next(t for t in package['targets'] if t['name'] == c['target'])
                if not Path(target['src_path']).resolve() == (ROOT / c['driver']).resolve():
                    raise AssertionError()
                if not any((build['command'][i:i + 2] == ['--bin', c['target']] for i in range(len(build['command']) - 1))):
                    raise AssertionError()
    if not len(record['contracts']) == len(registry['contracts']):
        raise AssertionError()
    if not {c['name'] for c in record['contracts']} == {c['name'] for c in registry['contracts']}:
        raise AssertionError()
    results = {c['name']: c for c in record['contracts']}
    for c in registry['contracts']:
        corpus = read(ROOT / c['corpus'])
        if not digest(ROOT / c['corpus']) == c['corpus_sha256']:
            raise AssertionError()
        check_corpus_binding(corpus, c)
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        if not (corpus['baseline'] == registry['runtime_commit'] and len(inputs) == c['count']):
            raise AssertionError()
        if c['historical_corpus']:
            if not digest(ROOT / c['historical_corpus']) == c['historical_sha256'] == corpus[c['historical_hash_field']]:
                raise AssertionError()
            if not same(inputs, [row['input'] for row in read(ROOT / c['historical_corpus'])['cases']]):
                raise AssertionError()
        result = results[c['name']]
        if not result['count'] == c['count']:
            raise AssertionError()
        payload = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in inputs)
        original = record['steps'][result['original_step']]
        native = record['steps'][result['native_step']]
        if not original['command'] == [str(owned / 'target/debug' / c['target'])]:
            raise AssertionError()
        if not native['command'] == [str(ROOT / 'build_native_current' / c['native_target'])]:
            raise AssertionError()
        for step in (original, native):
            if not step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest():
                raise AssertionError()
            if not step['stdin_bytes'] == len(payload.encode()):
                raise AssertionError()
            observed = json_lines(step['stdout'])
            if not (len(observed) == c['count'] and same(observed, expected)):
                raise AssertionError(c['name'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError()
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
