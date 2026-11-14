#!/usr/bin/env python3
"""Replay inherited-business adapters while preserving the original compiler defect."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

import archive_driver_provenance_5_0_6 as archive
import inherited_business_source_transition_5_0_6 as transition
from generated_driver_provenance_5_0_6 import inputs

ROOT = archive.ROOT
PACKAGES = {'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
            'teaql-macros', 'teaql-provider-sqlite'}


def check_inputs(registry):
    if not (registry['schema'] == 6 and registry['input_helpers'] == {}):
        raise AssertionError
    if not (set(registry['runtime_packages']) == PACKAGES):
        raise AssertionError
    for name, digest in registry['verifiers'].items():
        if not (archive.digest(ROOT / name) == digest):
            raise AssertionError
    transition.verify_support(registry['support_file_hashes'], {
        name: archive.digest(ROOT / name) for name in registry['support_files']})
    if not (len(set(registry['support_files'])) == len(registry['support_files'])):
        raise AssertionError
    if not ({'upstream.lock.json', registry['adapter']} <= set(registry['support_files'])):
        raise AssertionError
    for name in archive.source_files(registry):
        if not (not Path(name).is_absolute() and '..' not in Path(name).parts):
            raise AssertionError
    producer_lock = ROOT.parent / 'teaql_code_gen_cpp/cpp_generator.lock.json'
    if not (archive.digest(producer_lock) == registry['producer_lock_sha256']):
        raise AssertionError
    if not (archive.read(producer_lock)['generator_baseline_commit'] == registry['generator_commit']):
        raise AssertionError
    legacy_lock = ROOT / 'tests/parity/rust_inherited_business_compile/Cargo.lock'
    if not (archive.digest(legacy_lock) == registry['legacy_original_lock_sha256']):
        raise AssertionError
    if not (len(archive.registry_identities(legacy_lock) - archive.registry_identities(
        ROOT / '.reference/teaql-rs/Cargo.lock')) == registry['legacy_original_lock_differences'] == 46):
        raise AssertionError
    fixture = ROOT / registry['original_fixture']
    producer = archive.read(fixture / 'PROVENANCE.json')
    if not (producer['hand_edited'] is False):
        raise AssertionError
    if not (producer['runtime_commit'] == registry['runtime_commit']):
        raise AssertionError
    if not (producer['generator_commit'] == registry['generator_commit']):
        raise AssertionError
    if not (archive.digest(ROOT / producer['model']) == producer['model_sha256']):
        raise AssertionError
    required = {registry['original_fixture'] + '/' + name for name in producer['files']}
    required.update([registry['original_fixture'] + '/PROVENANCE.json', producer['model']])
    if not (required <= set(registry['support_files'])):
        raise AssertionError
    for name, digest in producer['files'].items():
        if not (archive.digest(fixture / name) == digest):
            raise AssertionError
    bridge = archive.read(ROOT / registry['bridge_report'])
    if not (bridge['original_files_unchanged'] is True):
        raise AssertionError
    if not (bridge['source_artifact_acceptance'] is False):
        raise AssertionError
    if not (bridge['untouched_producer_acceptance'] is False):
        raise AssertionError
    if not (bridge['inheritance_guard_removed'] is False):
        raise AssertionError
    if not (len(bridge['files']) == 43):
        raise AssertionError
    if not ({name for name, value in bridge['files'].items()
            if value['original_sha256'] != value['adapted_sha256']} == {
        'src/document/entity.rs', 'src/intermediate_record/entity.rs', 'src/invoice/entity.rs',
        'src/receipt/entity.rs', 'src/special_invoice/entity.rs'}):
        raise AssertionError
    failure = archive.read(ROOT / registry['failure_report'])
    if not (failure['exit_code'] == 101 and len(failure['errors']) == 20):
        raise AssertionError
    if not ({error['code'] for error in failure['errors']} == {'E0425', 'E0433'}):
        raise AssertionError
    if not (registry['adapted_source_only'] is True):
        raise AssertionError
    if not (registry['normalization'] == 'none'):
        raise AssertionError
    if not (registry['untouched_producer_acceptance'] is False):
        raise AssertionError
    if not (registry['inheritance_guard_removed'] is False):
        raise AssertionError
    contracts = registry['contracts']
    if not (len(contracts) == 2 and len({c['name'] for c in contracts}) == 2):
        raise AssertionError
    for contract in contracts:
        corpus = archive.read(ROOT / contract['corpus'])
        if not (archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']):
            raise AssertionError
        if not (archive.digest(ROOT / contract['driver']) == contract['driver_sha256']):
            raise AssertionError
        if not (corpus['baseline'] == registry['runtime_commit']):
            raise AssertionError
        if not (corpus['generator_commit'] == registry['generator_commit']):
            raise AssertionError
        if not (corpus['untouched_producer_acceptance'] is False):
            raise AssertionError
        if not (corpus['observer_sha256'] == contract['driver_sha256']):
            raise AssertionError
        if not (corpus['lock_sha256'] == archive.digest((ROOT / registry['project']).with_name('Cargo.lock'))):
            raise AssertionError
        if not (contract['count'] == len(corpus['cases'])):
            raise AssertionError
        builder = contract['input_builder']
        if not (builder['kind'] == 'function'
                and archive.digest(ROOT / builder['path']) == builder['sha256']
                == contract['input_recipe_sha256']):
            raise AssertionError
        if not (archive.same(inputs(contract), [row['input'] for row in corpus['cases']])):
            raise AssertionError


def compiler_errors(stdout, repository):
    errors = []
    for line in stdout.splitlines():
        value = json.loads(line)
        if value.get('reason') != 'compiler-message' or value['message']['level'] != 'error':
            continue
        message = value['message']
        spans = []
        for span in message['spans']:
            if span['is_primary']:
                path = Path(span['file_name'])
                path = path if path.is_absolute() else repository / path
                spans.append(dict(file=path.resolve().relative_to(repository).as_posix(),
                                  line=span['line_start']))
        errors.append(dict(code=message['code']['code'] if message['code'] else None,
                           message=message['message'], primary=spans))
    return sorted(errors, key=lambda error: json.dumps(error, sort_keys=True))


def check_metadata(record, registry, original=False):
    repository = Path(record['owned_directory']) / 'repository'
    key = 'original_metadata_step' if original else 'metadata_step'
    project = repository / (registry['original_project'] if original else registry['project'])
    step = record['steps'][record[key]]
    if not (step['command'] == ['cargo', 'metadata', '--offline', '--locked',
                               '--format-version', '1', '--manifest-path', str(project)]):
        raise AssertionError
    packages = json.loads(step['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    if not (package['name'] == (registry['original_package'] if original else registry['package'])):
        raise AssertionError
    targets = registry['original_targets'] if original else registry['targets']
    if not ({t['name'] for t in package['targets']} == set(targets)):
        raise AssertionError
    for target in package['targets']:
        if not (target['kind'] == ['bin']):
            raise AssertionError
        if not (Path(target['src_path']).resolve() == (repository / targets[target['name']]).resolve()):
            raise AssertionError
    fixture = registry['original_fixture'] if original else registry['derived_fixture']
    allowed = {project, repository / fixture / 'lib/Cargo.toml'}
    allowed |= {repository / '.reference/teaql-rs' / name / 'Cargo.toml' for name in PACKAGES}
    if not ({Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed):
        raise AssertionError
    if not ({p['name'] for p in packages if p['name'] in PACKAGES} == PACKAGES):
        raise AssertionError
    if not (all(p['version'] == '5.0.6' for p in packages if p['name'] in PACKAGES)):
        raise AssertionError
    lock = (ROOT / (registry['original_project'] if original else registry['project'])).with_name('Cargo.lock')
    if not (record['original_compiled_lock' if original else 'compiled_lock'] == lock.read_text()):
        raise AssertionError
    identities = archive.registry_identities(lock)
    if not (identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')):
        raise AssertionError
    if not ({(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (name, version, source) for name, version, source, checksum in identities}):
        raise AssertionError


def replay(registry, output):
    transition.require_replay_mode()
    check_inputs(registry)
    if not (not output.exists()):
        raise AssertionError
    record = dict(complete=False, whole_goal_complete=False, registry=registry, steps=[], results=[],
                  source_files=archive.source_files(registry), input_helpers={},
                  input_hashes_before=archive.capture_inputs(), references_before=archive.references(),
                  adapted_source_only=True, untouched_producer_acceptance=False,
                  source_artifact_acceptance=False, inheritance_guard_removed=False,
                  normalization='none')
    environment = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1',
                       TEAQL_BUSINESS_ID_KEY_VERSION='1',
                       TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())

    def save():
        output.write_bytes(gzip.compress(json.dumps(record).encode(), compresslevel=1, mtime=0))

    def run(label, command, payload=None, expected=0, cwd=ROOT):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=cwd, env=environment, input=payload,
                                capture_output=True, text=True)
        index = len(record['steps'])
        record['steps'].append(dict(label=label, command=command, exit_code=result.returncode,
            stdout=result.stdout, stderr=result.stderr,
            stdin_sha256=None if payload is None else hashlib.sha256(payload.encode()).hexdigest(),
            stdin_bytes=None if payload is None else len(payload.encode())))
        save()
        print(label, 'exit', result.returncode, flush=True)
        if not (result.returncode == expected):
            raise AssertionError((label, result.returncode, expected))
        return result.stdout, index

    try:
        run('upstream', ['python3', 'scripts/verify_upstream.py'])
        run('cargo', ['cargo', '--version'])
        run('rustc', ['rustc', '-vV'])
        run('native_build', ['cmake', '--build', '--preset', 'native', '--parallel', '4',
                            '--target', *[c['native_target'] for c in registry['contracts']]])
        data = archive.archive_bytes(registry)
        originals = archive.archive_files(data)
        record['archive_sha256'] = hashlib.sha256(data).hexdigest()
        record['archive_files'] = {name: hashlib.sha256(value).hexdigest()
                                   for name, value in originals.items()}
        with tempfile.TemporaryDirectory(prefix='teaql_inherited_business_driver_506_') as temporary:
            owned = Path(temporary)
            repository = owned / 'repository'
            record['owned_directory'] = temporary
            (owned / 'children').mkdir()
            environment.update(CARGO_TARGET_DIR=str(owned / 'target'), TMPDIR=str(owned / 'children'))
            record['cargo_environment'] = {key: environment[key] for key in [
                'CARGO_TARGET_DIR', 'CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG',
                'CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER', 'TMPDIR',
                'TEAQL_BUSINESS_ID_KEY_VERSION', 'TEAQL_BUSINESS_ID_KEY_V1_HEX']}
            original = repository / '.reference/teaql-rs'
            for name, content in originals.items():
                destination = original / name
                if not (destination.resolve().is_relative_to(original)):
                    raise AssertionError
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(content)
            for name in record['source_files']:
                destination = repository / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, destination)
            lock = owned / 'teaql_code_gen_cpp/cpp_generator.lock.json'
            lock.parent.mkdir()
            shutil.copyfile(ROOT.parent / 'teaql_code_gen_cpp/cpp_generator.lock.json', lock)
            original_project = repository / registry['original_project']
            _, record['original_metadata_step'] = run('original_metadata', ['cargo', 'metadata',
                '--offline', '--locked', '--format-version', '1', '--manifest-path', original_project])
            stdout, record['failure_step'] = run('original_expected_failure', ['cargo', 'check',
                '--offline', '--locked', '--manifest-path', original_project,
                '--message-format=json', '-j2'], expected=101, cwd=repository)
            record['original_errors'] = compiler_errors(stdout, repository)
            expected = archive.read(ROOT / registry['failure_report'])['errors']
            if not (record['original_errors'] == sorted(expected, key=lambda e: json.dumps(e, sort_keys=True))):
                raise AssertionError
            record['original_compiled_lock'] = original_project.with_name('Cargo.lock').read_text()
            _, record['adapter_step'] = run('original_adapter',
                ['python3', repository / registry['adapter']], cwd=repository)
            bridge_path = repository / registry['derived_fixture'] / 'provenance.json'
            record['adaptation_report'] = archive.read(bridge_path)
            expected = archive.read(ROOT / registry['bridge_report'])
            for key in ['files', 'slot_body_sha256', 'adaptation', 'original_files_unchanged',
                        'untouched_producer_acceptance', 'runtime_commit', 'generator_commit']:
                if not (record['adaptation_report'][key] == expected[key]):
                    raise AssertionError
            derived = repository / registry['derived_fixture'] / 'lib'
            record['derived_files'] = {str(p.relative_to(derived)): archive.digest(p)
                                       for p in derived.rglob('*') if p.is_file()}
            if not (record['derived_files'] == {name: value['adapted_sha256']
                                               for name, value in expected['files'].items()}):
                raise AssertionError
            project = repository / registry['project']
            metadata, record['metadata_step'] = run('locked_metadata', ['cargo', 'metadata',
                '--offline', '--locked', '--format-version', '1', '--manifest-path', project])
            record['project_metadata'] = metadata
            _, record['build_step'] = run('locked_build', ['cargo', 'build', '--offline', '--locked',
                '--manifest-path', project, '--bins', '-j2'])
            record['compiled_lock'] = project.with_name('Cargo.lock').read_text()
            raw, record['helper_step'] = run('five_model_helpers',
                [owned / 'target/debug' / registry['helper_target']])
            if not (archive.same(json.loads(raw), expected['output'])):
                raise AssertionError
            for contract in registry['contracts']:
                corpus = archive.read(ROOT / contract['corpus'])
                payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n'
                                  for row in corpus['cases'])
                expected_rows = [row['expected'] for row in corpus['cases']]
                binary = owned / 'target/debug' / contract['target']
                raw, source_step = run(contract['name'] + '_source', [binary], payload)
                if not (archive.same(archive.json_lines(raw), expected_rows)):
                    raise AssertionError
                raw, native_step = run(contract['name'] + '_native', contract['native_command'], payload)
                if not (archive.same(archive.json_lines(raw), expected_rows)):
                    raise AssertionError
                record['results'].append(dict(name=contract['name'], count=contract['count'],
                    source_step=source_step, native_step=native_step,
                    source_binary_sha256=archive.digest(binary),
                    native_binary_sha256=archive.digest(Path(contract['native_command'][0]))))
                save()
            if not (record['source_files'] == {name: archive.digest(repository / name)
                                               for name in record['source_files']}):
                raise AssertionError
            if not (record['archive_files'] == {name: archive.digest(original / name) for name in originals}):
                raise AssertionError
            if not (archive.digest(lock) == registry['producer_lock_sha256']):
                raise AssertionError
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = archive.capture_inputs()
        record['references_after'] = archive.references()
        if not (record['input_hashes_before'] == record['input_hashes_after']):
            raise AssertionError
        if not (record['references_before'] == record['references_after']):
            raise AssertionError
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        save()
    validate(record, registry)
    print('Original 20-error guard, exact derived bridge and 80 source/corpus/native outcomes verify.')


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
    source_mode = transition.verify_source_files(record['source_files'],
                                                 archive.source_files(registry))
    for name in [str(Path(__file__).relative_to(ROOT)), *registry['verifiers']]:
        transition.verify_runtime_hash(name, record['input_hashes_after']['runtime:' + name],
                                       archive.digest(ROOT / name), source_mode)
    if not (record['input_helpers'] == {}):
        raise AssertionError
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_inherited_business_driver_506_')):
        raise AssertionError
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError
    if not (record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'),
        TEAQL_BUSINESS_ID_KEY_VERSION='1', TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())):
        raise AssertionError
    if not (record['adapted_source_only'] is True):
        raise AssertionError
    if not (record['normalization'] == 'none'):
        raise AssertionError
    for key in ['untouched_producer_acceptance', 'source_artifact_acceptance', 'inheritance_guard_removed']:
        if not (record[key] is False):
            raise AssertionError
    steps = record['steps']
    if not (all(step['exit_code'] == (101 if i == record['failure_step'] else 0)
               for i, step in enumerate(steps))):
        raise AssertionError
    repository = owned / 'repository'
    original_project = repository / registry['original_project']
    if not (steps[record['failure_step']]['command'] == ['cargo', 'check', '--offline', '--locked',
        '--manifest-path', str(original_project), '--message-format=json', '-j2']):
        raise AssertionError
    expected_errors = sorted(archive.read(ROOT / registry['failure_report'])['errors'],
                             key=lambda error: json.dumps(error, sort_keys=True))
    if not (compiler_errors(steps[record['failure_step']]['stdout'], repository) == record['original_errors'] == expected_errors):
        raise AssertionError
    if not (steps[record['adapter_step']]['command'] == ['python3', str(repository / registry['adapter'])]):
        raise AssertionError
    bridge = archive.read(ROOT / registry['bridge_report'])
    for key in ['files', 'slot_body_sha256', 'adaptation', 'original_files_unchanged',
                'untouched_producer_acceptance', 'runtime_commit', 'generator_commit']:
        if not (record['adaptation_report'][key] == bridge[key]):
            raise AssertionError
    if not (record['derived_files'] == {name: value['adapted_sha256'] for name, value in bridge['files'].items()}):
        raise AssertionError
    data = archive.archive_bytes(registry)
    if not (record['archive_sha256'] == hashlib.sha256(data).hexdigest()):
        raise AssertionError
    if not (record['archive_files'] == {name: hashlib.sha256(value).hexdigest()
                                       for name, value in archive.archive_files(data).items()}):
        raise AssertionError
    check_metadata(record, registry, original=True)
    check_metadata(record, registry)
    if not (record['project_metadata'] == steps[record['metadata_step']]['stdout']):
        raise AssertionError
    project = repository / registry['project']
    if not (steps[record['build_step']]['command'] == ['cargo', 'build', '--offline', '--locked',
        '--manifest-path', str(project), '--bins', '-j2']):
        raise AssertionError
    helper = steps[record['helper_step']]
    if not (helper['command'] == [str(owned / 'target/debug' / registry['helper_target'])]):
        raise AssertionError
    if not (archive.same(json.loads(helper['stdout']), bridge['output'])):
        raise AssertionError
    if not (len(record['results']) == len(registry['contracts'])):
        raise AssertionError
    for contract, result in zip(registry['contracts'], record['results']):
        corpus = archive.read(ROOT / contract['corpus'])
        if not (result['name'] == contract['name'] and result['count'] == contract['count']):
            raise AssertionError
        if not (contract['native_command'] == [str(ROOT / 'build_native_current' / contract['native_target'])]):
            raise AssertionError
        if not (contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == contract['native_command'][0]):
            raise AssertionError
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected_rows = [row['expected'] for row in corpus['cases']]
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            if not (step['command'] == command):
                raise AssertionError
            if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
                raise AssertionError
            if not (step['stdin_bytes'] == len(payload.encode())):
                raise AssertionError
            if not (archive.same(archive.json_lines(step['stdout']), expected_rows)):
                raise AssertionError


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['replay', 'verify'])
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    registry = archive.read(args.contracts)
    if args.mode == 'replay':
        if not (args.record is not None):
            raise AssertionError
        replay(registry, args.record)
    else:
        if not (args.execution is not None and (args.record is None or not args.record.exists())):
            raise AssertionError
        validate(archive.read(args.execution), registry)
        if args.record:
            result = dict(complete=True, whole_goal_complete=False, adapted_source_only=True,
                source_contracts=2, observations=80, original_errors=20,
                execution_sha256=archive.digest(args.execution), registry_sha256=archive.digest(args.contracts))
            args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
        print('Adapted inherited-business bindings verify; untouched producer acceptance remains false.')


if __name__ == '__main__':
    main()
