#!/usr/bin/env python3
"""Verify compiled models, immutable source artifacts and bounded entity harnesses."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

import archive_driver_provenance_5_0_6 as archive

ROOT = archive.ROOT
RUNTIME_PACKAGES = {
    'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
    'teaql-macros', 'teaql-provider-sqlite',
}


from generated_driver_provenance_5_0_6 import inputs


def check_inputs(registry):
    assert registry['schema'] == 4
    assert registry['base_verifier_sha256'] == archive.digest(ROOT / 'scripts/generated_driver_provenance_5_0_6.py')
    assert set(registry['runtime_packages']) == RUNTIME_PACKAGES
    assert registry['input_helpers'] == {}  # The shared runner's SQL recipe mode is unused.
    support = set(registry['support_files'])
    assert len(support) == len(registry['support_files'])
    assert registry['support_file_hashes'] == {n: archive.digest(ROOT / n) for n in support}
    for name in [registry['project'], *support, *(c['driver'] for c in registry['contracts'])]:
        path = Path(name)
        assert not path.is_absolute() and '..' not in path.parts
        assert (ROOT / path).is_file()
    provenances = []
    for fixture in registry['generated_fixtures'] + registry['artifact_fixtures']:
        path = ROOT / fixture
        provenance = archive.read(path / 'PROVENANCE.json')
        assert provenance['runtime_commit'] == registry['runtime_commit']
        assert provenance['generator_commit'] == registry['generator_commit']
        assert provenance['hand_edited'] is False
        assert archive.digest(ROOT / provenance['model']) == provenance['model_sha256']
        required = {fixture + '/' + n for n in provenance['files']}
        required.update([fixture + '/PROVENANCE.json', provenance['model']])
        assert required <= support
        for name, expected in provenance['files'].items():
            assert archive.digest(path / name) == expected
        provenances.append(provenance)
    assert not set(registry['generated_fixtures']) & set(registry['artifact_fixtures'])
    library = registry['library_target']
    assert library['path'] in support
    assert archive.digest(ROOT / library['path']) == library['sha256']
    contracts = registry['contracts']
    assert contracts and len(contracts) == len({c['name'] for c in contracts})
    assert len({c['native_test'] for c in contracts}) == len(contracts)
    assert len({c['corpus'] for c in contracts}) == len(contracts)
    for contract in contracts:
        assert type(contract['count']) is int and contract['count'] > 0
        assert contract['input_builder']['path'] in support
        corpus = archive.read(ROOT / contract['corpus'])
        assert archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']
        assert corpus['baseline'] == registry['runtime_commit']
        assert archive.digest(ROOT / contract['driver']) == contract['driver_sha256']
        assert contract['native_command'][0] == str(ROOT / 'build_native_current' / contract['native_target'])
        assert archive.same(inputs(contract), [r['input'] for r in corpus['cases']])
        for key in ['producer', 'producers']:
            if key not in corpus:
                continue
            declarations = corpus[key]
            required = [declarations] if 'model' in declarations else list(declarations.values())
            assert required and all(value in provenances for value in required)
        source_root = contract.get('corpus_source_root', 'runtime')
        assert source_root in ['runtime', 'project']
        source_root = ROOT / '.reference/teaql-rs' if source_root == 'runtime' else ROOT
        for key in ['sources', 'source_hashes', 'source_sha256']:
            hashes = corpus.get(key)
            if isinstance(hashes, dict):
                for name, expected in hashes.items():
                    assert not Path(name).is_absolute() and '..' not in Path(name).parts
                    assert archive.digest(source_root / name) == expected
        if isinstance(corpus.get('source'), str) and isinstance(corpus.get('source_sha256'), str):
            assert archive.digest(source_root / corpus['source']) == corpus['source_sha256']
        for key in ['fixtures', 'fixture_hashes']:
            if isinstance(corpus.get(key), dict):
                fixture_root = ROOT / contract.get('corpus_fixture_root', 'tests/support')
                for name, expected in corpus[key].items():
                    assert archive.digest(fixture_root / name) == expected

def validate(record, registry):
    check_inputs(registry)
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    for script in [__file__, archive.__file__, ROOT / 'scripts/generated_driver_provenance_5_0_6.py']:
        path = str(Path(script).relative_to(ROOT))
        assert record['input_hashes_after']['runtime:' + path] == archive.digest(ROOT / path)
    assert record['source_files'] == archive.source_files(registry)
    assert record['input_helpers'] == {}
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_archive_driver_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))
    steps = record['steps']
    assert steps and all(s['exit_code'] == 0 for s in steps)
    data = archive.archive_bytes(registry)
    assert record['archive_sha256'] == hashlib.sha256(data).hexdigest()
    originals = archive.archive_files(data)
    assert record['archive_files'] == {n: hashlib.sha256(v).hexdigest() for n, v in originals.items()}
    assert all((ROOT / '.reference/teaql-rs' / n).read_bytes() == v for n, v in originals.items())
    project = owned / registry['project']
    metadata = steps[record['metadata_step']]
    assert metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project)]
    assert metadata['stdout'] == record['project_metadata']
    packages = json.loads(metadata['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    assert package['name'] == registry['package']
    contracts = registry['contracts']
    library = registry['library_target']
    assert {t['name'] for t in package['targets']} == {c['target'] for c in contracts} | {library['name']}
    target = next(t for t in package['targets'] if t['name'] == library['name'])
    assert target['kind'] == ['lib'] and Path(target['src_path']).resolve() == (owned / library['path']).resolve()
    assert all(t['kind'] == ['bin'] for t in package['targets'] if t['name'] != library['name'])
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        assert Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()
    allowed = {project} | {owned / fixture / 'lib/Cargo.toml' for fixture in registry['generated_fixtures']}
    allowed |= {owned / '.reference/teaql-rs' / path for path in registry['archive_manifests']}
    assert {n + '/Cargo.toml' for n in RUNTIME_PACKAGES} <= set(registry['archive_manifests'])
    assert {Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed
    assert all(p['version'] == '5.0.6' for p in packages if p['name'] in RUNTIME_PACKAGES)
    lock = (ROOT / registry['project']).with_name('Cargo.lock')
    assert record['compiled_lock'] == lock.read_text()
    identities = archive.registry_identities(lock)
    assert identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    assert {(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (n, v, s) for n, v, s, checksum in identities}
    build = steps[record['build_step']]
    assert build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']
    assert len(record['results']) == len(contracts)
    assert [r['name'] for r in record['results']] == [c['name'] for c in contracts]
    for contract, result in zip(contracts, record['results']):
        corpus = archive.read(ROOT / contract['corpus'])
        assert len(corpus['cases']) == contract['count'] == result['count']
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected = [row['expected'] for row in corpus['cases']]
        native = str(ROOT / 'build_native_current' / contract['native_target'])
        assert contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == native
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            assert step['command'] == command
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            assert archive.same(archive.json_lines(step['stdout']), expected)


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
        assert args.record is not None
        archive.replay(registry, args.record)
        validate(archive.read(args.record), registry)
    else:
        assert args.execution is not None
        assert args.record is None or not args.record.exists()
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
