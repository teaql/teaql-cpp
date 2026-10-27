#!/usr/bin/env python3
"""Replay and validate data-driven observer projects with an untouched locked archive."""
import argparse
from copy import deepcopy
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from run_native_integration_epoch_5_0_6 import capture_inputs, references
from verify_constant_closure_5_0_6 import registry_identities


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def source_files(registry):
    names = {c['driver'] for c in registry['contracts']} | set(registry['support_files'])
    names.update([registry['project'], str(Path(registry['project']).with_name('Cargo.lock'))])
    return {n: digest(ROOT / n) for n in sorted(names)}


def archive_bytes(registry):
    return subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'),
                                    'archive', registry['runtime_commit']])


def archive_files(data):
    result = {}
    with tarfile.open(fileobj=io.BytesIO(data)) as stream:
        for member in stream:
            if member.isfile():
                result[member.name] = stream.extractfile(member).read()
            else:
                assert member.isdir()
    return result


def check_input_recipes(registry):
    """Retain complete input derivations; never transform execution results."""
    helpers = registry.get('input_helpers', {})
    assert helpers == {n: digest(ROOT / n) for n in helpers}
    if not helpers:
        return
    from mutation_execution_inputs_5_0_6 import cases, cache_cases
    expected = {'mutation_execution_source': cases(), 'mutation_descriptor_cache': cache_cases()}
    historical = registry['historical_partition']
    path = ROOT / historical['path']
    assert digest(path) == historical['sha256']
    partition = []
    for row in read(path)['cases']:
        value = row['input']
        query = value['query']
        if query['group'] != ['owner', 'category'] or 'filter' in query or query['slice'] not in (
                {'offset': 0, 'limit': 1}, {'offset': 1, 'limit': 1}):
            continue
        queries = [deepcopy(query) for _ in range(3)]
        minimum = queries[0]['having']['right']['value']['value']
        queries[1]['having']['right']['value']['value'] = '3' if minimum == '2' else '2'
        queries[2]['having']['right']['value']['value'] = '1'
        partition.append(dict(seed_sql=value['seed_sql'], entity=value['entity'], queries=queries))
    expected['sql_cached_partition'] = partition
    assert len(partition) == 8
    for contract in registry['contracts']:
        corpus = read(ROOT / contract['corpus'])
        assert same([row['input'] for row in corpus['cases']], expected[contract['name']])
        if contract['name'] == 'sql_cached_partition':
            assert corpus['historical_corpus'] == path.name
            assert corpus['historical_sha256'] == historical['sha256']
            assert (corpus['historical_inputs'], corpus['explicit_intent_controls'],
                    corpus['negative_controls']) == (0, 8, 0)


def replay(registry, output):
    assert not output.exists()
    check_input_recipes(registry)
    record = dict(complete=False, whole_goal_complete=False, registry=registry, steps=[], results=[],
                  input_hashes_before=capture_inputs(), references_before=references(),
                  source_files=source_files(registry), input_helpers=registry.get('input_helpers', {}),
                  free_bytes_before=shutil.disk_usage(ROOT).free)
    environment = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1')

    def save():
        output.write_bytes(gzip.compress(json.dumps(record).encode(), compresslevel=1, mtime=0))

    def run(label, command, payload=None):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, env=environment, input=payload,
                                capture_output=True, text=True)
        index = len(record['steps'])
        record['steps'].append(dict(label=label, command=command, exit_code=result.returncode,
            stdout=result.stdout, stderr=result.stderr,
            stdin_sha256=None if payload is None else hashlib.sha256(payload.encode()).hexdigest(),
            stdin_bytes=None if payload is None else len(payload.encode())))
        save()
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout, index

    try:
        run('upstream', ['python3', 'scripts/verify_upstream.py'])
        run('cargo', ['cargo', '--version'])
        run('rustc', ['rustc', '-vV'])
        run('native_build', ['cmake', '--build', '--preset', 'native', '--parallel', '4',
            '--target', *dict.fromkeys(c['native_target'] for c in registry['contracts'])])
        data = archive_bytes(registry)
        originals = archive_files(data)
        record['archive_sha256'] = hashlib.sha256(data).hexdigest()
        record['archive_files'] = {n: hashlib.sha256(v).hexdigest() for n, v in originals.items()}
        with tempfile.TemporaryDirectory(prefix='teaql_archive_driver_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = temporary
            (owned / 'children').mkdir()
            environment.update(CARGO_TARGET_DIR=str(owned / 'target'), TMPDIR=str(owned / 'children'))
            record['cargo_environment'] = {k: environment[k] for k in [
                'CARGO_TARGET_DIR', 'CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG',
                'CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER', 'TMPDIR']}
            original = owned / '.reference/teaql-rs'
            for name, content in originals.items():
                destination = original / name
                assert destination.resolve().is_relative_to(original)
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(content)
            for name, value in record['source_files'].items():
                destination = owned / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, destination)
                assert digest(destination) == value
            project = owned / registry['project']
            metadata, record['metadata_step'] = run('locked_metadata', ['cargo', 'metadata',
                '--offline', '--locked', '--format-version', '1', '--manifest-path', project])
            record['project_metadata'] = metadata
            _, record['build_step'] = run('locked_build', ['cargo', 'build', '--offline', '--locked',
                '--manifest-path', project, '--bins', '-j2'])
            record['compiled_lock'] = project.with_name('Cargo.lock').read_text()
            for contract in registry['contracts']:
                corpus = read(ROOT / contract['corpus'])
                assert digest(ROOT / contract['corpus']) == contract['corpus_sha256']
                inputs = [row['input'] for row in corpus['cases']]
                expected = [row['expected'] for row in corpus['cases']]
                payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
                binary = owned / 'target/debug' / contract['target']
                raw, source_step = run(contract['name'] + '_source', [binary], payload)
                assert same(json_lines(raw), expected), contract['name'] + ': source differs'
                raw, native_step = run(contract['name'] + '_native', contract['native_command'], payload)
                assert same(json_lines(raw), expected), contract['name'] + ': native differs'
                record['results'].append(dict(name=contract['name'], count=contract['count'],
                    source_step=source_step, native_step=native_step,
                    source_binary_sha256=digest(binary),
                    native_binary_sha256=digest(Path(contract['native_command'][0]))))
                save()
            assert record['source_files'] == {n: digest(owned / n) for n in record['source_files']}
            assert record['archive_files'] == {n: digest(original / n) for n in originals}
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = capture_inputs()
        record['references_after'] = references()
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert record['references_before'] == record['references_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        save()
    print('All', len(registry['contracts']), 'complete source/corpus/native bindings agree.')


def validate(record, registry):
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    script = str(Path(__file__).relative_to(ROOT))
    assert record['input_hashes_after']['runtime:' + script] == digest(ROOT / script)
    contracts = registry['contracts']
    assert contracts and len(contracts) == len({c['name'] for c in contracts})
    assert all(type(c['count']) is int and c['count'] > 0 for c in contracts)
    assert record['source_files'] == source_files(registry)
    assert record['input_helpers'] == registry.get('input_helpers', {})
    check_input_recipes(registry)
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_archive_driver_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))
    steps = record['steps']
    assert steps and all(s['exit_code'] == 0 for s in steps)
    data = archive_bytes(registry)
    assert record['archive_sha256'] == hashlib.sha256(data).hexdigest()
    originals = archive_files(data)
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
    assert {t['name'] for t in package['targets']} == {c['target'] for c in contracts}
    assert all(t['kind'] == ['bin'] for t in package['targets'])
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        assert Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()
    allowed = {project} | {owned / '.reference/teaql-rs' / n / 'Cargo.toml'
                           for n in registry['runtime_packages']}
    assert {Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed
    assert all(p['version'] == '5.0.6' for p in packages if p['name'] in registry['runtime_packages'])
    lock = (ROOT / registry['project']).with_name('Cargo.lock')
    assert record['compiled_lock'] == lock.read_text()
    identities = registry_identities(lock)
    assert identities <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    assert {(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (n, v, s) for n, v, s, checksum in identities}
    build = steps[record['build_step']]
    assert build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']
    assert len(record['results']) == len(contracts)
    assert [r['name'] for r in record['results']] == [c['name'] for c in contracts]
    for contract, result in zip(contracts, record['results']):
        corpus = read(ROOT / contract['corpus'])
        assert digest(ROOT / contract['corpus']) == contract['corpus_sha256']
        assert digest(ROOT / contract['driver']) == contract['driver_sha256']
        assert corpus['baseline'] == registry['runtime_commit']
        assert corpus['driver'] == contract['driver'] and corpus['driver_sha256'] == contract['driver_sha256']
        assert len(corpus['cases']) == contract['count'] == result['count']
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected = [row['expected'] for row in corpus['cases']]
        native = ROOT / 'build_native_current' / contract['native_target']
        assert contract['native_command'] == [str(native)]
        assert contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == str(native)
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            assert step['command'] == command
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            assert same(json_lines(step['stdout']), expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['replay', 'verify'])
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    registry = read(args.contracts)
    if args.mode == 'replay':
        assert args.record is not None
        replay(registry, args.record)
    else:
        assert args.execution is not None and (args.record is None or not args.record.exists())
        validate(read(args.execution), registry)
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True,
                whole_goal_complete=False, source_contracts=len(registry['contracts']),
                compiled_targets=len({c['target'] for c in registry['contracts']}),
                observations=sum(c['count'] for c in registry['contracts']),
                execution_sha256=digest(args.execution), registry_sha256=digest(args.contracts))).encode(), mtime=0))
        print('Complete archive, project, dependency, input-recipe and raw-result bindings verify.')


if __name__ == '__main__':
    main()
