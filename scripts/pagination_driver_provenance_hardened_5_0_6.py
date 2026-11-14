#!/usr/bin/env python3
"""Bind five retained pagination corpora to fresh, untouched Rust execution."""
import argparse
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

import archive_driver_provenance_hardened_5_0_6 as archive

ROOT = archive.ROOT
HISTORICAL_DRIVER = ROOT / 'scripts/pagination_driver_provenance_5_0_6.py'
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
    if not (registry['schema'] == 2 and registry['project'] == PROJECT):
        raise AssertionError
    if not (registry['package'] == 'teaql_pagination_original_source_506'):
        raise AssertionError
    if not (set(registry['runtime_packages']) == RUNTIME_PACKAGES):
        raise AssertionError
    if not (registry['input_helpers'] == {}):
        raise AssertionError  # The shared runner's SQL recipe mode is unused.
    if not (registry['generated_fixture'] == FIXTURE):
        raise AssertionError
    provenance = archive.read(ROOT / FIXTURE / 'PROVENANCE.json')
    if not (provenance['runtime_commit'] == registry['runtime_commit']):
        raise AssertionError
    if not (provenance['generator_commit'] == registry['generator_commit']):
        raise AssertionError
    if not (provenance['hand_edited'] is False):
        raise AssertionError
    if not (archive.digest(ROOT / provenance['model']) == provenance['model_sha256']):
        raise AssertionError
    for name, expected in provenance['files'].items():
        if not (archive.digest(ROOT / FIXTURE / name) == expected):
            raise AssertionError
    support = {FIXTURE + '/' + name for name in provenance['files']}
    support.update([FIXTURE + '/PROVENANCE.json', provenance['model'],
        'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
        'tests/support/dynamic_json_schema.sql', 'tests/support/dynamic_json_dataset.sql'])
    contracts = registry['contracts']
    if not ([c['name'] for c in contracts] == list(RECIPES)):
        raise AssertionError
    if not (len({c['target'] for c in contracts}) == 4):
        raise AssertionError
    for contract in contracts:
        name = contract['name']
        helper, function, count = RECIPES[name]
        helper = 'tests/parity/' + helper
        support.add(helper)
        if not (contract['input_recipe'] == helper and contract['input_function'] == function):
            raise AssertionError
        if not (contract['input_recipe_sha256'] == archive.digest(ROOT / helper)):
            raise AssertionError
        spec = importlib.util.spec_from_file_location('_pagination_recipe_' + name, ROOT / helper)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        inputs = list(getattr(module, function)())
        corpus = archive.read(ROOT / contract['corpus'])
        if not (archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']):
            raise AssertionError
        if not (archive.same(inputs, [row['input'] for row in corpus['cases']])):
            raise AssertionError
        if not (len(inputs) == contract['count'] == count):
            raise AssertionError
        if not (corpus['baseline'] == registry['runtime_commit']):
            raise AssertionError
        if not (corpus['producer'] == provenance):
            raise AssertionError
        driver_name = 'generated_execution' if name == 'pagination_convenience' else name
        if not (contract['target'] == driver_name + '_5_0_6'):
            raise AssertionError
        if not (contract['driver'] == 'tests/parity/rust_field_layout/src/bin/' + contract['target'] + '.rs'):
            raise AssertionError
        if not (archive.digest(ROOT / contract['driver']) == contract['driver_sha256']):
            raise AssertionError
        if not (contract['corpus'] == 'tests/parity/' + name + '_5_0_6_cases.json.gz'):
            raise AssertionError
        if not (contract['native_test'] == name + '_rust_parity'):
            raise AssertionError
        if not (contract['native_target'] == 'teaql_' + driver_name + '_probe'):
            raise AssertionError
        if not (contract['native_command'] == [str(ROOT / 'build_native_current' / contract['native_target'])]):
            raise AssertionError
    if not (set(registry['support_files']) == support):
        raise AssertionError
    if not (len(registry['support_files']) == len(support)):
        raise AssertionError
    fixtures = {name: archive.digest(ROOT / 'tests/support' / name)
                for name in ['dynamic_json_schema.sql', 'dynamic_json_dataset.sql']}
    if not (archive.read(ROOT / contracts[-1]['corpus'])['fixtures'] == fixtures):
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
    for script in [HISTORICAL_DRIVER, ROOT / 'scripts/archive_driver_provenance_5_0_6.py']:
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
    if not ({t['name'] for t in package['targets']} == {c['target'] for c in contracts}):
        raise AssertionError
    if not (all(t['kind'] == ['bin'] for t in package['targets'])):
        raise AssertionError
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        if not (Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()):
            raise AssertionError
    allowed = {project, owned / FIXTURE / 'lib/Cargo.toml'} | {
        owned / '.reference/teaql-rs' / n / 'Cargo.toml' for n in RUNTIME_PACKAGES}
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
            raise RuntimeError('Source-driver archive replay requires assertions in shared runners')
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
            result = dict(complete=True, whole_goal_complete=False, source_contracts=5,
                compiled_targets=4, observations=10888, execution_sha256=archive.digest(args.execution),
                registry_sha256=archive.digest(args.contracts))
            args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print('Five pagination input/source/corpus/native bindings verify: 10,888 observations.')


if __name__ == '__main__':
    main()
