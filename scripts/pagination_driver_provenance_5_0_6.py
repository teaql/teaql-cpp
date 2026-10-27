#!/usr/bin/env python3
"""Bind five retained pagination corpora to fresh, untouched Rust execution."""
import argparse
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path

import archive_driver_provenance_5_0_6 as archive

ROOT = archive.ROOT
FIXTURE = 'tests/generator/parity/dynamic_json_rust'
PROJECT = 'tests/parity/rust_pagination_source_5_0_6/Cargo.toml'
RECIPES = {
    'deferred_continuous_page': ('generate_deferred_continuous_page_5_0_6.py', 'inputs', 640),
    'deferred_plain_page': ('generate_deferred_plain_page_5_0_6.py', 'inputs', 108),
    'deferred_id_set_page': ('generate_deferred_id_set_page_5_0_6.py', 'inputs', 252),
    'pagination_convenience': ('generate_pagination_convenience_5_0_6.py', 'inputs', 8448),
    'generated_execution': ('generate_generated_execution.py', 'cases', 1440),
}
RUNTIME_PACKAGES = {
    'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
    'teaql-macros', 'teaql-provider-sqlite',
}


def check_inputs(registry):
    assert registry['schema'] == 2 and registry['project'] == PROJECT
    assert registry['package'] == 'teaql_pagination_original_source_506'
    assert set(registry['runtime_packages']) == RUNTIME_PACKAGES
    assert registry['input_helpers'] == {}  # The shared runner's SQL recipe mode is unused.
    assert registry['generated_fixture'] == FIXTURE
    provenance = archive.read(ROOT / FIXTURE / 'PROVENANCE.json')
    assert provenance['runtime_commit'] == registry['runtime_commit']
    assert provenance['generator_commit'] == registry['generator_commit']
    assert provenance['hand_edited'] is False
    assert archive.digest(ROOT / provenance['model']) == provenance['model_sha256']
    for name, expected in provenance['files'].items():
        assert archive.digest(ROOT / FIXTURE / name) == expected
    support = {FIXTURE + '/' + name for name in provenance['files']}
    support.update([FIXTURE + '/PROVENANCE.json', provenance['model'],
        'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
        'tests/support/dynamic_json_schema.sql', 'tests/support/dynamic_json_dataset.sql'])
    contracts = registry['contracts']
    assert [c['name'] for c in contracts] == list(RECIPES)
    assert len({c['target'] for c in contracts}) == 4
    for contract in contracts:
        name = contract['name']
        helper, function, count = RECIPES[name]
        helper = 'tests/parity/' + helper
        support.add(helper)
        assert contract['input_recipe'] == helper and contract['input_function'] == function
        assert contract['input_recipe_sha256'] == archive.digest(ROOT / helper)
        spec = importlib.util.spec_from_file_location('_pagination_recipe_' + name, ROOT / helper)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        inputs = list(getattr(module, function)())
        corpus = archive.read(ROOT / contract['corpus'])
        assert archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']
        assert archive.same(inputs, [row['input'] for row in corpus['cases']])
        assert len(inputs) == contract['count'] == count
        assert corpus['baseline'] == registry['runtime_commit']
        assert corpus['producer'] == provenance
        driver_name = 'generated_execution' if name == 'pagination_convenience' else name
        assert contract['target'] == driver_name + '_5_0_6'
        assert contract['driver'] == 'tests/parity/rust_field_layout/src/bin/' + contract['target'] + '.rs'
        assert archive.digest(ROOT / contract['driver']) == contract['driver_sha256']
        assert contract['corpus'] == 'tests/parity/' + name + '_5_0_6_cases.json.gz'
        assert contract['native_test'] == name + '_rust_parity'
        assert contract['native_target'] == 'teaql_' + driver_name + '_probe'
        assert contract['native_command'] == [str(ROOT / 'build_native_current' / contract['native_target'])]
    assert set(registry['support_files']) == support
    assert len(registry['support_files']) == len(support)
    fixtures = {name: archive.digest(ROOT / 'tests/support' / name)
                for name in ['dynamic_json_schema.sql', 'dynamic_json_dataset.sql']}
    assert archive.read(ROOT / contracts[-1]['corpus'])['fixtures'] == fixtures


def validate(record, registry):
    check_inputs(registry)
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    for script in [__file__, archive.__file__]:
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
    assert {t['name'] for t in package['targets']} == {c['target'] for c in contracts}
    assert all(t['kind'] == ['bin'] for t in package['targets'])
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        assert Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()
    allowed = {project, owned / FIXTURE / 'lib/Cargo.toml'} | {
        owned / '.reference/teaql-rs' / n / 'Cargo.toml' for n in RUNTIME_PACKAGES}
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
            result = dict(complete=True, whole_goal_complete=False, source_contracts=5,
                compiled_targets=4, observations=10888, execution_sha256=archive.digest(args.execution),
                registry_sha256=archive.digest(args.contracts))
            args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print('Five pagination input/source/corpus/native bindings verify: 10,888 observations.')


if __name__ == '__main__':
    main()
