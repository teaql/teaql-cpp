#!/usr/bin/env python3
"""Check complete private-runtime source, project, extraction and output bindings."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from replay_private_runtime_drivers_5_0_6 import (
    builder, builder_inputs, digest, expected_extractions, read)
from verify_constant_closure_5_0_6 import registry_identities
from replay_relation_hydration_intent_5_0_6 import inputs as hydration_intent_inputs


def historical_inputs(contract, record):
    historical = read(ROOT / contract['historical_corpus'])
    name = contract['name']
    if name in ['runtime_base_intent', 'context_query_intent']:
        adapter = 'tests/parity/replay_layered_query_intent_5_0_6.py'
        if not (record['input_hashes_after']['runtime:' + adapter] == digest(ROOT / adapter)):
            raise AssertionError
        return [dict(row['input'], purpose=purpose) for row in historical['cases']
                for purpose in ['Inspect layered query intent', ' ', '  Inspect layered query intent  ']]
    if name == 'relation_hydration_intent':
        adapter = 'tests/parity/replay_relation_hydration_intent_5_0_6.py'
        if not (record['input_hashes_after']['runtime:' + adapter] == digest(ROOT / adapter)):
            raise AssertionError
        if not (contract['historical_corpus'] == 'tests/parity/relation_hydration_cases.json'):
            raise AssertionError
        return hydration_intent_inputs()
    return [row['input'] for row in historical['cases']]


def validate(record, registry):
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError
    if not (record['registry'] == registry):
        raise AssertionError
    if not (registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']):
        raise AssertionError
    if not (registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']):
        raise AssertionError
    if not (record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}):
        raise AssertionError
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError
    replay = 'scripts/replay_private_runtime_drivers_5_0_6.py'
    if not (record['input_hashes_after']['runtime:' + replay] == digest(ROOT / replay)):
        raise AssertionError
    if not (record['builder_inputs'] == builder_inputs()):
        raise AssertionError
    if not (record['appendices'] == builder.APPENDS and len(builder.APPENDS) == 9):
        raise AssertionError
    contracts = registry['contracts'] + registry['repeated_contracts']
    if not (len(registry['contracts']) == 36 and sum(c['count'] for c in registry['contracts']) == 34846):
        raise AssertionError
    if not (len(registry['repeated_contracts']) == 3 and sum(c['count'] for c in registry['repeated_contracts']) == 27078):
        raise AssertionError
    if not (len({c['name'] for c in contracts}) == len(contracts) == 39):
        raise AssertionError
    if not ({c['target'] for c in contracts} == {n + '_5_0_6' for n in builder.FAMILIES}):
        raise AssertionError
    if not (len({c['target'] for c in contracts}) == 34):
        raise AssertionError
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_private_runtime_driver_506_')):
        raise AssertionError
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError
    if not (record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')):
        raise AssertionError
    steps = record['steps']
    if not (steps and all(s['exit_code'] == 0 for s in steps)):
        raise AssertionError
    reference = ROOT / '.reference/teaql-rs'
    archive = subprocess.check_output(['git', '-C', str(reference), 'archive', registry['runtime_commit']])
    if not (record['archive_sha256'] == hashlib.sha256(archive).hexdigest()):
        raise AssertionError
    expected_archive = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if member.isfile():
                prefix = stream.extractfile(member).read()
                if not ((reference / member.name).read_bytes() == prefix):
                    raise AssertionError
                appendix = builder.APPENDS.get(member.name, '').encode()
                expected_archive[member.name] = dict(original_sha256=hashlib.sha256(prefix).hexdigest(),
                    compiled_sha256=hashlib.sha256(prefix + appendix).hexdigest(), appended_bytes=len(appendix))
    if not (record['archive_files'] == expected_archive):
        raise AssertionError
    source_manifest = ROOT / registry['project']
    if not (record['project_files'] == {n: source_manifest.with_name(n).read_text()
                                       for n in ['Cargo.toml', 'Cargo.lock', 'build.rs']}):
        raise AssertionError
    project = owned / 'tests/parity/rust_private_save_5_0_6'
    original = owned / 'build_rust_private_save_5_0_6/original'
    if not (record['source_links'] == {'src': str(source_manifest.parent / 'src'),
        'build.rs': str(source_manifest.parent / 'build.rs'),
        '../rust_values': str(ROOT / 'tests/parity/rust_values')}):
        raise AssertionError
    fixtures = ['tests/parity/rust_values/src/fixture_value.rs',
                'tests/parity/rust_values/src/fixture_sql.rs']
    if not (record['fixture_inputs'] == {p: digest(ROOT / p) for p in fixtures}):
        raise AssertionError
    metadata = steps[record['metadata_step']]
    if not (metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project / 'Cargo.toml')]):
        raise AssertionError
    package_data = json.loads(metadata['stdout'])
    package = next(p for p in package_data['packages'] if p['manifest_path'] == str(project / 'Cargo.toml'))
    targets = {t['name']: t for t in package['targets']}
    if not ({t['name'] for t in package['targets'] if 'bin' in t['kind']} == {c['target'] for c in contracts}):
        raise AssertionError
    if not (set(record['resolved_target_paths']) == set(targets)):
        raise AssertionError
    for name, target in targets.items():
        if 'custom-build' in target['kind']:
            if not (target['src_path'] == str(project / 'build.rs')):
                raise AssertionError
            if not (record['resolved_target_paths'][name] == str(source_manifest.with_name('build.rs'))):
                raise AssertionError
        else:
            if not (target['src_path'] == str(project / 'src/bin' / (name + '.rs'))):
                raise AssertionError
            if not (record['resolved_target_paths'][name] == str(source_manifest.parent / 'src/bin' / (name + '.rs'))):
                raise AssertionError
    for item in package_data['packages']:
        if item['source'] is None:
            if not (Path(item['manifest_path']) == project / 'Cargo.toml' or Path(item['manifest_path']).is_relative_to(original)):
                raise AssertionError
    with tempfile.TemporaryDirectory(prefix='teaql_private_runtime_lock_506_') as temporary:
        lock = Path(temporary) / 'Cargo.lock'
        for content in [record['project_files']['Cargo.lock'], record['compiled_lock']]:
            lock.write_text(content)
            if not (registry_identities(lock) <= registry_identities(reference / 'Cargo.lock')):
                raise AssertionError
        compiled_packages = {(name, version, source) for name, version, source, checksum
                             in registry_identities(lock)}
        metadata_packages = {(p['name'], p['version'], p['source'])
                             for p in package_data['packages'] if p['source'] is not None}
        if not (metadata_packages and metadata_packages <= compiled_packages):
            raise AssertionError
    build = steps[record['build_step']]
    if not (build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project / 'Cargo.toml'), '--bins', '-j2']):
        raise AssertionError
    extractions = expected_extractions()
    if not (record['extracted_files'] == {name: dict(text=value,
        sha256=hashlib.sha256(value.encode()).hexdigest()) for name, value in extractions.items()}):
        raise AssertionError
    if not (len(record['results']) == 39):
        raise AssertionError
    if not ([r['name'] for r in record['results']] == [c['name'] for c in contracts]):
        raise AssertionError
    for c, result in zip(contracts, record['results']):
        corpus = read(ROOT / c['corpus'])
        if not (digest(ROOT / c['corpus']) == c['corpus_sha256']):
            raise AssertionError
        if not (digest(ROOT / c['driver']) == c['driver_sha256']):
            raise AssertionError
        if not (corpus['baseline'] == registry['runtime_commit']):
            raise AssertionError
        if not (corpus['driver'] == c['driver'] and corpus['driver_sha256'] == c['driver_sha256']):
            raise AssertionError
        if not (c['driver'] == str((source_manifest.parent / 'src/bin' / (c['target'] + '.rs')).relative_to(ROOT))):
            raise AssertionError
        if not (type(c['count']) is int and len(corpus['cases']) == c['count'] > 0):
            raise AssertionError
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        if c['historical_corpus']:
            if not (digest(ROOT / c['historical_corpus']) == c['historical_sha256'] == corpus['historical_corpus_sha256']):
                raise AssertionError
            if not (same(inputs, historical_inputs(c, record))):
                raise AssertionError
        if not (result['count'] == c['count']):
            raise AssertionError
        payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
        native = ROOT / 'build_native_current' / c['native_target']
        if not (c['native_command'] == [str(native)]):
            raise AssertionError
        if not (c['registered_command'][c['registered_command'].index('--cpp') + 1] == str(native)):
            raise AssertionError
        for key, command in [('source_step', [str(owned / 'target/debug' / c['target'])]),
                             ('native_step', c['native_command'])]:
            step = steps[result[key]]
            if not (step['command'] == command):
                raise AssertionError
            if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
                raise AssertionError
            if not (step['stdin_bytes'] == len(payload.encode())):
                raise AssertionError
            if not (same(json_lines(step['stdout']), expected)):
                raise AssertionError


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError
    validate(read(args.execution), read(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True,
            whole_goal_complete=False, compiled_targets=34, matrices=39,
            new_contracts=36, new_observations=34846, repeated_observations=27078,
            execution_sha256=digest(args.execution), registry_sha256=digest(args.contracts))).encode(), mtime=0))
    print('All 34 private-runtime targets and 39 raw source/corpus/native bindings verify.')


if __name__ == '__main__':
    main()
