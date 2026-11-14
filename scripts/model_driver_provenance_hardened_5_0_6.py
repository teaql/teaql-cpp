#!/usr/bin/env python3
"""Verify compiled models, immutable source artifacts and bounded entity harnesses."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sys

import archive_driver_provenance_5_0_6 as archive

ROOT = archive.ROOT
HISTORICAL_DRIVER = ROOT / 'scripts/model_driver_provenance_5_0_6.py'
RUNTIME_PACKAGES = {
    'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
    'teaql-macros', 'teaql-provider-sqlite',
}


from generated_driver_provenance_hardened_5_0_6 import inputs


def check_inputs(registry):
    if not (registry['schema'] == 4):
        raise AssertionError
    if not (registry['base_verifier_sha256'] == archive.digest(ROOT / 'scripts/generated_driver_provenance_5_0_6.py')):
        raise AssertionError
    if not (set(registry['runtime_packages']) == RUNTIME_PACKAGES):
        raise AssertionError
    if not (registry['input_helpers'] == {}):  # The shared runner's SQL recipe mode is unused.
        raise AssertionError
    support = set(registry['support_files'])
    if not (len(support) == len(registry['support_files'])):
        raise AssertionError
    if not (registry['support_file_hashes'] == {n: archive.digest(ROOT / n) for n in support}):
        raise AssertionError
    for name in [registry['project'], *support, *(c['driver'] for c in registry['contracts'])]:
        path = Path(name)
        if not (not path.is_absolute() and '..' not in path.parts):
            raise AssertionError
        if not ((ROOT / path).is_file()):
            raise AssertionError
    provenances = []
    for fixture in registry['generated_fixtures'] + registry['artifact_fixtures']:
        path = ROOT / fixture
        provenance = archive.read(path / 'PROVENANCE.json')
        if not (provenance['runtime_commit'] == registry['runtime_commit']):
            raise AssertionError
        if not (provenance['generator_commit'] == registry['generator_commit']):
            raise AssertionError
        if not (provenance['hand_edited'] is False):
            raise AssertionError
        if not (archive.digest(ROOT / provenance['model']) == provenance['model_sha256']):
            raise AssertionError
        required = {fixture + '/' + n for n in provenance['files']}
        required.update([fixture + '/PROVENANCE.json', provenance['model']])
        if not (required <= support):
            raise AssertionError
        for name, expected in provenance['files'].items():
            if not (archive.digest(path / name) == expected):
                raise AssertionError
        provenances.append(provenance)
    if not (not set(registry['generated_fixtures']) & set(registry['artifact_fixtures'])):
        raise AssertionError
    library = registry['library_target']
    if not (library['path'] in support):
        raise AssertionError
    if not (archive.digest(ROOT / library['path']) == library['sha256']):
        raise AssertionError
    contracts = registry['contracts']
    if not (contracts and len(contracts) == len({c['name'] for c in contracts})):
        raise AssertionError
    if not (len({c['native_test'] for c in contracts}) == len(contracts)):
        raise AssertionError
    if not (len({c['corpus'] for c in contracts}) == len(contracts)):
        raise AssertionError
    for contract in contracts:
        if not (type(contract['count']) is int and contract['count'] > 0):
            raise AssertionError
        if not (contract['input_builder']['path'] in support):
            raise AssertionError
        corpus = archive.read(ROOT / contract['corpus'])
        if not (archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']):
            raise AssertionError
        if not (corpus['baseline'] == registry['runtime_commit']):
            raise AssertionError
        if not (archive.digest(ROOT / contract['driver']) == contract['driver_sha256']):
            raise AssertionError
        if not (contract['native_command'][0] == str(ROOT / 'build_native_current' / contract['native_target'])):
            raise AssertionError
        if not (archive.same(inputs(contract), [r['input'] for r in corpus['cases']])):
            raise AssertionError
        for key in ['producer', 'producers']:
            if key not in corpus:
                continue
            declarations = corpus[key]
            required = [declarations] if 'model' in declarations else list(declarations.values())
            if not (required and all(value in provenances for value in required)):
                raise AssertionError
        source_root = contract.get('corpus_source_root', 'runtime')
        if not (source_root in ['runtime', 'project']):
            raise AssertionError
        source_root = ROOT / '.reference/teaql-rs' if source_root == 'runtime' else ROOT
        for key in ['sources', 'source_hashes', 'source_sha256']:
            hashes = corpus.get(key)
            if isinstance(hashes, dict):
                for name, expected in hashes.items():
                    if not (not Path(name).is_absolute() and '..' not in Path(name).parts):
                        raise AssertionError
                    if not (archive.digest(source_root / name) == expected):
                        raise AssertionError
        if isinstance(corpus.get('source'), str) and isinstance(corpus.get('source_sha256'), str):
            if not (archive.digest(source_root / corpus['source']) == corpus['source_sha256']):
                raise AssertionError
        for key in ['fixtures', 'fixture_hashes']:
            if isinstance(corpus.get(key), dict):
                fixture_root = ROOT / contract.get('corpus_fixture_root', 'tests/support')
                for name, expected in corpus[key].items():
                    if not (archive.digest(fixture_root / name) == expected):
                        raise AssertionError

def validate(record, registry):
    check_inputs(registry)
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError
    if not (record['registry'] == registry):
        raise AssertionError
    if not (registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']):
        raise AssertionError
    if not (registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']):
        raise AssertionError
    if not (record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}):
        raise AssertionError
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError
    for script in [HISTORICAL_DRIVER, archive.__file__, ROOT / 'scripts/generated_driver_provenance_5_0_6.py']:
        path = str(Path(script).relative_to(ROOT))
        if not (record['input_hashes_after']['runtime:' + path] == archive.digest(ROOT / path)):
            raise AssertionError
    if not (record['source_files'] == archive.source_files(registry)):
        raise AssertionError
    if not (record['input_helpers'] == {}):
        raise AssertionError
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_archive_driver_506_')):
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
    data = archive.archive_bytes(registry)
    if not (record['archive_sha256'] == hashlib.sha256(data).hexdigest()):
        raise AssertionError
    originals = archive.archive_files(data)
    if not (record['archive_files'] == {n: hashlib.sha256(v).hexdigest() for n, v in originals.items()}):
        raise AssertionError
    if not (all((ROOT / '.reference/teaql-rs' / n).read_bytes() == v for n, v in originals.items())):
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
    if not (package['name'] == registry['package']):
        raise AssertionError
    contracts = registry['contracts']
    library = registry['library_target']
    if not ({t['name'] for t in package['targets']} == {c['target'] for c in contracts} | {library['name']}):
        raise AssertionError
    target = next(t for t in package['targets'] if t['name'] == library['name'])
    if not (target['kind'] == ['lib'] and Path(target['src_path']).resolve() == (owned / library['path']).resolve()):
        raise AssertionError
    if not (all(t['kind'] == ['bin'] for t in package['targets'] if t['name'] != library['name'])):
        raise AssertionError
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        if not (Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()):
            raise AssertionError
    allowed = {project} | {owned / fixture / 'lib/Cargo.toml' for fixture in registry['generated_fixtures']}
    allowed |= {owned / '.reference/teaql-rs' / path for path in registry['archive_manifests']}
    if not ({n + '/Cargo.toml' for n in RUNTIME_PACKAGES} <= set(registry['archive_manifests'])):
        raise AssertionError
    if not ({Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed):
        raise AssertionError
    if not (all(p['version'] == '5.0.6' for p in packages if p['name'] in RUNTIME_PACKAGES)):
        raise AssertionError
    lock = (ROOT / registry['project']).with_name('Cargo.lock')
    if not (record['compiled_lock'] == lock.read_text()):
        raise AssertionError
    identities = archive.registry_identities(lock)
    if not (identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')):
        raise AssertionError
    if not ({(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (n, v, s) for n, v, s, checksum in identities}):
        raise AssertionError
    build = steps[record['build_step']]
    if not (build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']):
        raise AssertionError
    if not (len(record['results']) == len(contracts)):
        raise AssertionError
    if not ([r['name'] for r in record['results']] == [c['name'] for c in contracts]):
        raise AssertionError
    for contract, result in zip(contracts, record['results']):
        corpus = archive.read(ROOT / contract['corpus'])
        if not (len(corpus['cases']) == contract['count'] == result['count']):
            raise AssertionError
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected = [row['expected'] for row in corpus['cases']]
        native = str(ROOT / 'build_native_current' / contract['native_target'])
        if not (contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == native):
            raise AssertionError
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            if not (step['command'] == command):
                raise AssertionError
            if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
                raise AssertionError
            if not (step['stdin_bytes'] == len(payload.encode())):
                raise AssertionError
            if not (archive.same(archive.json_lines(step['stdout']), expected)):
                raise AssertionError


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['replay', 'verify'])
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    registry = archive.read(args.contracts)
    check_inputs(registry)
    if args.mode == 'replay':
        if sys.flags.optimize != 0:
            raise RuntimeError('Model archive replay requires assertions in its shared runner')
        if not (args.record is not None):
            raise AssertionError
        archive.replay(registry, args.record)
        validate(archive.read(args.record), registry)
    else:
        if not (args.execution is not None):
            raise AssertionError
        if not (args.record is None or not args.record.exists()):
            raise AssertionError
        validate(archive.read(args.execution), registry)
        if args.record:
            result = dict(complete=True, whole_goal_complete=False,
                source_contracts=len(registry['contracts']),
                compiled_targets=len({c['target'] for c in registry['contracts']}),
                observations=sum(c['count'] for c in registry['contracts']),
                execution_sha256=archive.digest(args.execution),
                registry_sha256=archive.digest(args.contracts))
            args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print('Complete generated-model source/input/dependency/result bindings verify.')


if __name__ == '__main__':
    main()
