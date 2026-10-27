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
        assert record['input_hashes_after']['runtime:' + adapter] == digest(ROOT / adapter)
        return [dict(row['input'], purpose=purpose) for row in historical['cases']
                for purpose in ['Inspect layered query intent', ' ', '  Inspect layered query intent  ']]
    if name == 'relation_hydration_intent':
        adapter = 'tests/parity/replay_relation_hydration_intent_5_0_6.py'
        assert record['input_hashes_after']['runtime:' + adapter] == digest(ROOT / adapter)
        assert contract['historical_corpus'] == 'tests/parity/relation_hydration_cases.json'
        return hydration_intent_inputs()
    return [row['input'] for row in historical['cases']]


def validate(record, registry):
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    replay = 'scripts/replay_private_runtime_drivers_5_0_6.py'
    assert record['input_hashes_after']['runtime:' + replay] == digest(ROOT / replay)
    assert record['builder_inputs'] == builder_inputs()
    assert record['appendices'] == builder.APPENDS and len(builder.APPENDS) == 9
    contracts = registry['contracts'] + registry['repeated_contracts']
    assert len(registry['contracts']) == 36 and sum(c['count'] for c in registry['contracts']) == 34846
    assert len(registry['repeated_contracts']) == 3 and sum(c['count'] for c in registry['repeated_contracts']) == 27078
    assert len({c['name'] for c in contracts}) == len(contracts) == 39
    assert {c['target'] for c in contracts} == {n + '_5_0_6' for n in builder.FAMILIES}
    assert len({c['target'] for c in contracts}) == 34
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_private_runtime_driver_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    steps = record['steps']
    assert steps and all(s['exit_code'] == 0 for s in steps)
    reference = ROOT / '.reference/teaql-rs'
    archive = subprocess.check_output(['git', '-C', str(reference), 'archive', registry['runtime_commit']])
    assert record['archive_sha256'] == hashlib.sha256(archive).hexdigest()
    expected_archive = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if member.isfile():
                prefix = stream.extractfile(member).read()
                assert (reference / member.name).read_bytes() == prefix
                appendix = builder.APPENDS.get(member.name, '').encode()
                expected_archive[member.name] = dict(original_sha256=hashlib.sha256(prefix).hexdigest(),
                    compiled_sha256=hashlib.sha256(prefix + appendix).hexdigest(), appended_bytes=len(appendix))
    assert record['archive_files'] == expected_archive
    source_manifest = ROOT / registry['project']
    assert record['project_files'] == {n: source_manifest.with_name(n).read_text()
                                       for n in ['Cargo.toml', 'Cargo.lock', 'build.rs']}
    project = owned / 'tests/parity/rust_private_save_5_0_6'
    original = owned / 'build_rust_private_save_5_0_6/original'
    assert record['source_links'] == {'src': str(source_manifest.parent / 'src'),
        'build.rs': str(source_manifest.parent / 'build.rs'),
        '../rust_values': str(ROOT / 'tests/parity/rust_values')}
    fixtures = ['tests/parity/rust_values/src/fixture_value.rs',
                'tests/parity/rust_values/src/fixture_sql.rs']
    assert record['fixture_inputs'] == {p: digest(ROOT / p) for p in fixtures}
    metadata = steps[record['metadata_step']]
    assert metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project / 'Cargo.toml')]
    package_data = json.loads(metadata['stdout'])
    package = next(p for p in package_data['packages'] if p['manifest_path'] == str(project / 'Cargo.toml'))
    targets = {t['name']: t for t in package['targets']}
    assert {t['name'] for t in package['targets'] if 'bin' in t['kind']} == {c['target'] for c in contracts}
    assert set(record['resolved_target_paths']) == set(targets)
    for name, target in targets.items():
        if 'custom-build' in target['kind']:
            assert target['src_path'] == str(project / 'build.rs')
            assert record['resolved_target_paths'][name] == str(source_manifest.with_name('build.rs'))
        else:
            assert target['src_path'] == str(project / 'src/bin' / (name + '.rs'))
            assert record['resolved_target_paths'][name] == str(source_manifest.parent / 'src/bin' / (name + '.rs'))
    for item in package_data['packages']:
        if item['source'] is None:
            assert Path(item['manifest_path']) == project / 'Cargo.toml' or Path(item['manifest_path']).is_relative_to(original)
    with tempfile.TemporaryDirectory(prefix='teaql_private_runtime_lock_506_') as temporary:
        lock = Path(temporary) / 'Cargo.lock'
        for content in [record['project_files']['Cargo.lock'], record['compiled_lock']]:
            lock.write_text(content)
            assert registry_identities(lock) <= registry_identities(reference / 'Cargo.lock')
        compiled_packages = {(name, version, source) for name, version, source, checksum
                             in registry_identities(lock)}
        metadata_packages = {(p['name'], p['version'], p['source'])
                             for p in package_data['packages'] if p['source'] is not None}
        assert metadata_packages and metadata_packages <= compiled_packages
    build = steps[record['build_step']]
    assert build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project / 'Cargo.toml'), '--bins', '-j2']
    extractions = expected_extractions()
    assert record['extracted_files'] == {name: dict(text=value,
        sha256=hashlib.sha256(value.encode()).hexdigest()) for name, value in extractions.items()}
    assert len(record['results']) == 39
    assert [r['name'] for r in record['results']] == [c['name'] for c in contracts]
    for c, result in zip(contracts, record['results']):
        corpus = read(ROOT / c['corpus'])
        assert digest(ROOT / c['corpus']) == c['corpus_sha256']
        assert digest(ROOT / c['driver']) == c['driver_sha256']
        assert corpus['baseline'] == registry['runtime_commit']
        assert corpus['driver'] == c['driver'] and corpus['driver_sha256'] == c['driver_sha256']
        assert c['driver'] == str((source_manifest.parent / 'src/bin' / (c['target'] + '.rs')).relative_to(ROOT))
        assert type(c['count']) is int and len(corpus['cases']) == c['count'] > 0
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        if c['historical_corpus']:
            assert digest(ROOT / c['historical_corpus']) == c['historical_sha256'] == corpus['historical_corpus_sha256']
            assert same(inputs, historical_inputs(c, record))
        assert result['count'] == c['count']
        payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
        native = ROOT / 'build_native_current' / c['native_target']
        assert c['native_command'] == [str(native)]
        assert c['registered_command'][c['registered_command'].index('--cpp') + 1] == str(native)
        for key, command in [('source_step', [str(owned / 'target/debug' / c['target'])]),
                             ('native_step', c['native_command'])]:
            step = steps[result[key]]
            assert step['command'] == command
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            assert same(json_lines(step['stdout']), expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    validate(read(args.execution), read(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True,
            whole_goal_complete=False, compiled_targets=34, matrices=39,
            new_contracts=36, new_observations=34846, repeated_observations=27078,
            execution_sha256=digest(args.execution), registry_sha256=digest(args.contracts))).encode(), mtime=0))
    print('All 34 private-runtime targets and 39 raw source/corpus/native bindings verify.')


if __name__ == '__main__':
    main()
