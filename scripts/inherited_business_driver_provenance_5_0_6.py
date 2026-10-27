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
from generated_driver_provenance_5_0_6 import inputs

ROOT = archive.ROOT
PACKAGES = {'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
            'teaql-macros', 'teaql-provider-sqlite'}


def check_inputs(registry):
    assert registry['schema'] == 6 and registry['input_helpers'] == {}
    assert set(registry['runtime_packages']) == PACKAGES
    for name, digest in registry['verifiers'].items():
        assert archive.digest(ROOT / name) == digest
    assert registry['support_file_hashes'] == {
        name: archive.digest(ROOT / name) for name in registry['support_files']}
    assert len(set(registry['support_files'])) == len(registry['support_files'])
    assert {'upstream.lock.json', registry['adapter']} <= set(registry['support_files'])
    for name in archive.source_files(registry):
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
    producer_lock = ROOT.parent / 'teaql_code_gen_cpp/cpp_generator.lock.json'
    assert archive.digest(producer_lock) == registry['producer_lock_sha256']
    assert archive.read(producer_lock)['generator_baseline_commit'] == registry['generator_commit']
    legacy_lock = ROOT / 'tests/parity/rust_inherited_business_compile/Cargo.lock'
    assert archive.digest(legacy_lock) == registry['legacy_original_lock_sha256']
    assert len(archive.registry_identities(legacy_lock) - archive.registry_identities(
        ROOT / '.reference/teaql-rs/Cargo.lock')) == registry['legacy_original_lock_differences'] == 46
    fixture = ROOT / registry['original_fixture']
    producer = archive.read(fixture / 'PROVENANCE.json')
    assert producer['hand_edited'] is False
    assert producer['runtime_commit'] == registry['runtime_commit']
    assert producer['generator_commit'] == registry['generator_commit']
    assert archive.digest(ROOT / producer['model']) == producer['model_sha256']
    required = {registry['original_fixture'] + '/' + name for name in producer['files']}
    required.update([registry['original_fixture'] + '/PROVENANCE.json', producer['model']])
    assert required <= set(registry['support_files'])
    for name, digest in producer['files'].items():
        assert archive.digest(fixture / name) == digest
    bridge = archive.read(ROOT / registry['bridge_report'])
    assert bridge['original_files_unchanged'] is True
    assert bridge['source_artifact_acceptance'] is False
    assert bridge['untouched_producer_acceptance'] is False
    assert bridge['inheritance_guard_removed'] is False
    assert len(bridge['files']) == 43
    assert {name for name, value in bridge['files'].items()
            if value['original_sha256'] != value['adapted_sha256']} == {
        'src/document/entity.rs', 'src/intermediate_record/entity.rs', 'src/invoice/entity.rs',
        'src/receipt/entity.rs', 'src/special_invoice/entity.rs'}
    failure = archive.read(ROOT / registry['failure_report'])
    assert failure['exit_code'] == 101 and len(failure['errors']) == 20
    assert {error['code'] for error in failure['errors']} == {'E0425', 'E0433'}
    assert registry['adapted_source_only'] is True
    assert registry['normalization'] == 'none'
    assert registry['untouched_producer_acceptance'] is False
    assert registry['inheritance_guard_removed'] is False
    contracts = registry['contracts']
    assert len(contracts) == 2 and len({c['name'] for c in contracts}) == 2
    for contract in contracts:
        corpus = archive.read(ROOT / contract['corpus'])
        assert archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']
        assert archive.digest(ROOT / contract['driver']) == contract['driver_sha256']
        assert corpus['baseline'] == registry['runtime_commit']
        assert corpus['generator_commit'] == registry['generator_commit']
        assert corpus['untouched_producer_acceptance'] is False
        assert corpus['observer_sha256'] == contract['driver_sha256']
        assert corpus['lock_sha256'] == archive.digest((ROOT / registry['project']).with_name('Cargo.lock'))
        assert contract['count'] == len(corpus['cases'])
        assert archive.same(inputs(contract), [row['input'] for row in corpus['cases']])


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
    assert step['command'] == ['cargo', 'metadata', '--offline', '--locked',
                               '--format-version', '1', '--manifest-path', str(project)]
    packages = json.loads(step['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    assert package['name'] == (registry['original_package'] if original else registry['package'])
    targets = registry['original_targets'] if original else registry['targets']
    assert {t['name'] for t in package['targets']} == set(targets)
    for target in package['targets']:
        assert target['kind'] == ['bin']
        assert Path(target['src_path']).resolve() == (repository / targets[target['name']]).resolve()
    fixture = registry['original_fixture'] if original else registry['derived_fixture']
    allowed = {project, repository / fixture / 'lib/Cargo.toml'}
    allowed |= {repository / '.reference/teaql-rs' / name / 'Cargo.toml' for name in PACKAGES}
    assert {Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed
    assert {p['name'] for p in packages if p['name'] in PACKAGES} == PACKAGES
    assert all(p['version'] == '5.0.6' for p in packages if p['name'] in PACKAGES)
    lock = (ROOT / (registry['original_project'] if original else registry['project'])).with_name('Cargo.lock')
    assert record['original_compiled_lock' if original else 'compiled_lock'] == lock.read_text()
    identities = archive.registry_identities(lock)
    assert identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    assert {(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (name, version, source) for name, version, source, checksum in identities}


def replay(registry, output):
    check_inputs(registry)
    assert not output.exists()
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
        assert result.returncode == expected, (label, result.returncode, expected)
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
                assert destination.resolve().is_relative_to(original)
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
            assert record['original_errors'] == sorted(expected, key=lambda e: json.dumps(e, sort_keys=True))
            record['original_compiled_lock'] = original_project.with_name('Cargo.lock').read_text()
            _, record['adapter_step'] = run('original_adapter',
                ['python3', repository / registry['adapter']], cwd=repository)
            bridge_path = repository / registry['derived_fixture'] / 'provenance.json'
            record['adaptation_report'] = archive.read(bridge_path)
            expected = archive.read(ROOT / registry['bridge_report'])
            for key in ['files', 'slot_body_sha256', 'adaptation', 'original_files_unchanged',
                        'untouched_producer_acceptance', 'runtime_commit', 'generator_commit']:
                assert record['adaptation_report'][key] == expected[key]
            derived = repository / registry['derived_fixture'] / 'lib'
            record['derived_files'] = {str(p.relative_to(derived)): archive.digest(p)
                                       for p in derived.rglob('*') if p.is_file()}
            assert record['derived_files'] == {name: value['adapted_sha256']
                                               for name, value in expected['files'].items()}
            project = repository / registry['project']
            metadata, record['metadata_step'] = run('locked_metadata', ['cargo', 'metadata',
                '--offline', '--locked', '--format-version', '1', '--manifest-path', project])
            record['project_metadata'] = metadata
            _, record['build_step'] = run('locked_build', ['cargo', 'build', '--offline', '--locked',
                '--manifest-path', project, '--bins', '-j2'])
            record['compiled_lock'] = project.with_name('Cargo.lock').read_text()
            raw, record['helper_step'] = run('five_model_helpers',
                [owned / 'target/debug' / registry['helper_target']])
            assert archive.same(json.loads(raw), expected['output'])
            for contract in registry['contracts']:
                corpus = archive.read(ROOT / contract['corpus'])
                payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n'
                                  for row in corpus['cases'])
                expected_rows = [row['expected'] for row in corpus['cases']]
                binary = owned / 'target/debug' / contract['target']
                raw, source_step = run(contract['name'] + '_source', [binary], payload)
                assert archive.same(archive.json_lines(raw), expected_rows)
                raw, native_step = run(contract['name'] + '_native', contract['native_command'], payload)
                assert archive.same(archive.json_lines(raw), expected_rows)
                record['results'].append(dict(name=contract['name'], count=contract['count'],
                    source_step=source_step, native_step=native_step,
                    source_binary_sha256=archive.digest(binary),
                    native_binary_sha256=archive.digest(Path(contract['native_command'][0]))))
                save()
            assert record['source_files'] == {name: archive.digest(repository / name)
                                               for name in record['source_files']}
            assert record['archive_files'] == {name: archive.digest(original / name) for name in originals}
            assert archive.digest(lock) == registry['producer_lock_sha256']
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = archive.capture_inputs()
        record['references_after'] = archive.references()
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert record['references_before'] == record['references_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        save()
    validate(record, registry)
    print('Original 20-error guard, exact derived bridge and 80 source/corpus/native outcomes verify.')


def validate(record, registry):
    check_inputs(registry)
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    for name in [str(Path(__file__).relative_to(ROOT)), *registry['verifiers']]:
        assert record['input_hashes_after']['runtime:' + name] == archive.digest(ROOT / name)
    assert record['source_files'] == archive.source_files(registry) and record['input_helpers'] == {}
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_inherited_business_driver_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'),
        TEAQL_BUSINESS_ID_KEY_VERSION='1', TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
    assert record['adapted_source_only'] is True
    assert record['normalization'] == 'none'
    for key in ['untouched_producer_acceptance', 'source_artifact_acceptance', 'inheritance_guard_removed']:
        assert record[key] is False
    steps = record['steps']
    assert all(step['exit_code'] == (101 if i == record['failure_step'] else 0)
               for i, step in enumerate(steps))
    repository = owned / 'repository'
    original_project = repository / registry['original_project']
    assert steps[record['failure_step']]['command'] == ['cargo', 'check', '--offline', '--locked',
        '--manifest-path', str(original_project), '--message-format=json', '-j2']
    expected_errors = sorted(archive.read(ROOT / registry['failure_report'])['errors'],
                             key=lambda error: json.dumps(error, sort_keys=True))
    assert compiler_errors(steps[record['failure_step']]['stdout'], repository) == record['original_errors'] == expected_errors
    assert steps[record['adapter_step']]['command'] == ['python3', str(repository / registry['adapter'])]
    bridge = archive.read(ROOT / registry['bridge_report'])
    for key in ['files', 'slot_body_sha256', 'adaptation', 'original_files_unchanged',
                'untouched_producer_acceptance', 'runtime_commit', 'generator_commit']:
        assert record['adaptation_report'][key] == bridge[key]
    assert record['derived_files'] == {name: value['adapted_sha256'] for name, value in bridge['files'].items()}
    data = archive.archive_bytes(registry)
    assert record['archive_sha256'] == hashlib.sha256(data).hexdigest()
    assert record['archive_files'] == {name: hashlib.sha256(value).hexdigest()
                                       for name, value in archive.archive_files(data).items()}
    check_metadata(record, registry, original=True)
    check_metadata(record, registry)
    assert record['project_metadata'] == steps[record['metadata_step']]['stdout']
    project = repository / registry['project']
    assert steps[record['build_step']]['command'] == ['cargo', 'build', '--offline', '--locked',
        '--manifest-path', str(project), '--bins', '-j2']
    helper = steps[record['helper_step']]
    assert helper['command'] == [str(owned / 'target/debug' / registry['helper_target'])]
    assert archive.same(json.loads(helper['stdout']), bridge['output'])
    assert len(record['results']) == len(registry['contracts'])
    for contract, result in zip(registry['contracts'], record['results']):
        corpus = archive.read(ROOT / contract['corpus'])
        assert result['name'] == contract['name'] and result['count'] == contract['count']
        assert contract['native_command'] == [str(ROOT / 'build_native_current' / contract['native_target'])]
        assert contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == contract['native_command'][0]
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected_rows = [row['expected'] for row in corpus['cases']]
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            assert step['command'] == command
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            assert archive.same(archive.json_lines(step['stdout']), expected_rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['replay', 'verify'])
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    registry = archive.read(args.contracts)
    if args.mode == 'replay':
        assert args.record is not None
        replay(registry, args.record)
    else:
        assert args.execution is not None and (args.record is None or not args.record.exists())
        validate(archive.read(args.execution), registry)
        if args.record:
            result = dict(complete=True, whole_goal_complete=False, adapted_source_only=True,
                source_contracts=2, observations=80, original_errors=20,
                execution_sha256=archive.digest(args.execution), registry_sha256=archive.digest(args.contracts))
            args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
        print('Adapted inherited-business bindings verify; untouched producer acceptance remains false.')


if __name__ == '__main__':
    main()
