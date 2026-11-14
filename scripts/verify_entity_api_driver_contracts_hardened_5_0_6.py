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
    for name in ['scripts/replay_entity_api_drivers_5_0_6.py', registry['legacy_project'],
                 str(Path(registry['legacy_project']).with_name('Cargo.lock'))]:
        if not (record['input_hashes_after']['runtime:' + name] == digest(ROOT / name)):
            raise AssertionError
    contracts = registry['contracts']
    if not (len(contracts) == len({c['name'] for c in contracts}) == 28):
        raise AssertionError
    if not (sum(c['count'] for c in contracts) == 11022):
        raise AssertionError
    if not (len({c['target'] for c in contracts}) == 27):
        raise AssertionError
    if not (record['source_files'] == source_files(registry)):
        raise AssertionError
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_entity_api_driver_506_')):
        raise AssertionError
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError
    if not (record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))):
        raise AssertionError
    steps = record['steps']
    if not (steps and all(s['exit_code'] == 0 for s in steps)):
        raise AssertionError
    reference = ROOT / '.reference/teaql-rs'
    archive = subprocess.check_output(['git', '-C', str(reference), 'archive', registry['runtime_commit']])
    if not (record['archive_sha256'] == hashlib.sha256(archive).hexdigest()):
        raise AssertionError
    files = {}
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if member.isfile():
                data = stream.extractfile(member).read()
                if not ((reference / member.name).read_bytes() == data):
                    raise AssertionError
                files[member.name] = hashlib.sha256(data).hexdigest()
    if not (record['archive_files'] == files and len(files) == 610):
        raise AssertionError
    project = owned / registry['project']
    metadata = steps[record['metadata_step']]
    if not (metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project)]):
        raise AssertionError
    if not (metadata['stdout'] == record['project_metadata']):
        raise AssertionError
    packages = json.loads(metadata['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    if not (package['name'] == 'teaql_entity_api_original_source_506'):
        raise AssertionError
    if not ({t['name'] for t in package['targets']} == {c['target'] for c in contracts}):
        raise AssertionError
    if not (all(t['kind'] == ['bin'] for t in package['targets'])):
        raise AssertionError
    for c in contracts:
        target = next(t for t in package['targets'] if t['name'] == c['target'])
        if not (Path(target['src_path']).resolve() == (owned / c['driver']).resolve()):
            raise AssertionError
    allowed = {project}
    allowed.update(owned / directory / 'Cargo.toml' for directory in registry['generated_fixtures'].values())
    teaql_names = {'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
                   'teaql-macros', 'teaql-provider-sqlite'}
    allowed.update(owned / '.reference/teaql-rs' / n / 'Cargo.toml' for n in teaql_names)
    actual = {Path(p['manifest_path']) for p in packages if p['source'] is None}
    if not (actual == allowed):
        raise AssertionError
    if not (all(p['version'] == '5.0.6' for p in packages if p['name'] in teaql_names)):
        raise AssertionError
    lock = ROOT / registry['project']
    lock = lock.with_name('Cargo.lock')
    if not (record['compiled_lock'] == lock.read_text()):
        raise AssertionError
    identities = registry_identities(lock)
    if not (identities <= registry_identities(reference / 'Cargo.lock')):
        raise AssertionError
    resolved = {(n, v, s) for n, v, s, checksum in identities}
    if not ({(p['name'], p['version'], p['source']) for p in packages if p['source']} <= resolved):
        raise AssertionError
    build = steps[record['build_step']]
    if not (build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']):
        raise AssertionError
    if not (len(record['results']) == 28):
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
        if not (type(c['count']) is int and len(corpus['cases']) == c['count'] > 0):
            raise AssertionError
        if not (Path(c['driver']).stem == c['target']):
            raise AssertionError
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        if c['historical_corpus']:
            if not (digest(ROOT / c['historical_corpus']) == c['historical_sha256'] == corpus['historical_corpus_sha256']):
                raise AssertionError
            if not (same(inputs, [row['input'] for row in read(ROOT / c['historical_corpus'])['cases']])):
                raise AssertionError
        if not (result['count'] == c['count']):
            raise AssertionError
        native = ROOT / 'build_native_current' / c['native_target']
        if not (c['native_command'] == [str(native)]):
            raise AssertionError
        if not (c['registered_command'][c['registered_command'].index('--cpp') + 1] == str(native)):
            raise AssertionError
        payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
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
            whole_goal_complete=False, compiled_targets=27, source_contracts=28,
            observations=11022, generated_fixtures=4, execution_sha256=digest(args.execution),
            registry_sha256=digest(args.contracts))).encode(), mtime=0))
    print('All27 source targets and28 complete original/corpus/native bindings verify.')


if __name__ == '__main__':
    main()
