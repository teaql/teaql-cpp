#!/usr/bin/env python3
"""Validate the complete declared entity API worklist without rebuilding Rust."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from replay_entity_api_drivers_5_0_6 import digest, read, source_files
from verify_constant_closure_5_0_6 import registry_identities


def validate(record, registry):
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    for name in ['scripts/replay_entity_api_drivers_5_0_6.py', registry['legacy_project'],
                 str(Path(registry['legacy_project']).with_name('Cargo.lock'))]:
        assert record['input_hashes_after']['runtime:' + name] == digest(ROOT / name)
    contracts = registry['contracts']
    assert len(contracts) == len({c['name'] for c in contracts}) == 28
    assert sum(c['count'] for c in contracts) == 11022
    assert len({c['target'] for c in contracts}) == 27
    assert record['source_files'] == source_files(registry)
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_entity_api_driver_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))
    steps = record['steps']
    assert steps and all(s['exit_code'] == 0 for s in steps)
    reference = ROOT / '.reference/teaql-rs'
    archive = subprocess.check_output(['git', '-C', str(reference), 'archive', registry['runtime_commit']])
    assert record['archive_sha256'] == hashlib.sha256(archive).hexdigest()
    files = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if member.isfile():
                data = stream.extractfile(member).read()
                assert (reference / member.name).read_bytes() == data
                files[member.name] = hashlib.sha256(data).hexdigest()
    assert record['archive_files'] == files and len(files) == 610
    project = owned / registry['project']
    metadata = steps[record['metadata_step']]
    assert metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project)]
    assert metadata['stdout'] == record['project_metadata']
    packages = json.loads(metadata['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    assert package['name'] == 'teaql_entity_api_original_source_506'
    assert {t['name'] for t in package['targets']} == {c['target'] for c in contracts}
    assert all(t['kind'] == ['bin'] for t in package['targets'])
    for c in contracts:
        target = next(t for t in package['targets'] if t['name'] == c['target'])
        assert Path(target['src_path']).resolve() == (owned / c['driver']).resolve()
    allowed = {project}
    allowed.update(owned / directory / 'Cargo.toml' for directory in registry['generated_fixtures'].values())
    teaql_names = {'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
                   'teaql-macros', 'teaql-provider-sqlite'}
    allowed.update(owned / '.reference/teaql-rs' / n / 'Cargo.toml' for n in teaql_names)
    actual = {Path(p['manifest_path']) for p in packages if p['source'] is None}
    assert actual == allowed
    assert all(p['version'] == '5.0.6' for p in packages if p['name'] in teaql_names)
    lock = ROOT / registry['project']
    lock = lock.with_name('Cargo.lock')
    assert record['compiled_lock'] == lock.read_text()
    identities = registry_identities(lock)
    assert identities <= registry_identities(reference / 'Cargo.lock')
    resolved = {(n, v, s) for n, v, s, checksum in identities}
    assert {(p['name'], p['version'], p['source']) for p in packages if p['source']} <= resolved
    build = steps[record['build_step']]
    assert build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']
    assert len(record['results']) == 28
    assert [r['name'] for r in record['results']] == [c['name'] for c in contracts]
    for c, result in zip(contracts, record['results']):
        corpus = read(ROOT / c['corpus'])
        assert digest(ROOT / c['corpus']) == c['corpus_sha256']
        assert digest(ROOT / c['driver']) == c['driver_sha256']
        assert corpus['baseline'] == registry['runtime_commit']
        assert corpus['driver'] == c['driver'] and corpus['driver_sha256'] == c['driver_sha256']
        assert type(c['count']) is int and len(corpus['cases']) == c['count'] > 0
        assert Path(c['driver']).stem == c['target']
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        if c['historical_corpus']:
            assert digest(ROOT / c['historical_corpus']) == c['historical_sha256'] == corpus['historical_corpus_sha256']
            assert same(inputs, [row['input'] for row in read(ROOT / c['historical_corpus'])['cases']])
        assert result['count'] == c['count']
        native = ROOT / 'build_native_current' / c['native_target']
        assert c['native_command'] == [str(native)]
        assert c['registered_command'][c['registered_command'].index('--cpp') + 1] == str(native)
        payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
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
            whole_goal_complete=False, compiled_targets=27, source_contracts=28,
            observations=11022, generated_fixtures=4, execution_sha256=digest(args.execution),
            registry_sha256=digest(args.contracts))).encode(), mtime=0))
    print('All27 source targets and28 complete original/corpus/native bindings verify.')


if __name__ == '__main__':
    main()
