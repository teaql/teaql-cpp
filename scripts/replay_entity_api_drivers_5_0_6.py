#!/usr/bin/env python3
"""Replay all remaining declared field/entity API drivers in one locked source project."""
import argparse
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
    paths = {ROOT / c['driver'] for c in registry['contracts']}
    paths.update(ROOT / registry['project'].rsplit('/', 1)[0] / name
                 for name in ['Cargo.toml', 'Cargo.lock'])
    paths.update(p for p in (ROOT / 'tests/parity/rust_values/src').rglob('*') if p.is_file())
    for directory in registry['generated_fixtures'].values():
        paths.add(ROOT / directory / 'Cargo.toml')
        paths.update(p for p in (ROOT / directory / 'src').rglob('*') if p.is_file())
    return {str(p.relative_to(ROOT)): digest(p) for p in sorted(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    registry = read(args.contracts)
    contracts = registry['contracts']
    assert len(contracts) == 28 and len({c['target'] for c in contracts}) == 27
    assert sum(c['count'] for c in contracts) == 11022
    record = dict(complete=False, whole_goal_complete=False, registry=registry, steps=[], results=[],
                  input_hashes_before=capture_inputs(), references_before=references(),
                  source_files=source_files(registry), free_bytes_before=shutil.disk_usage(ROOT).free)
    environment = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1')

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

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
            '--target', *dict.fromkeys(c['native_target'] for c in contracts)])
        archive = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'),
                                           'archive', registry['runtime_commit']])
        record['archive_sha256'] = hashlib.sha256(archive).hexdigest()
        with tempfile.TemporaryDirectory(prefix='teaql_entity_api_driver_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = temporary
            (owned / 'children').mkdir()
            environment.update(CARGO_TARGET_DIR=str(owned / 'target'), TMPDIR=str(owned / 'children'))
            record['cargo_environment'] = {k: environment[k] for k in [
                'CARGO_TARGET_DIR', 'CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG',
                'CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER', 'TMPDIR']}
            original = owned / '.reference/teaql-rs'
            record['archive_files'] = {}
            with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
                for member in stream:
                    if not member.isfile():
                        assert member.isdir()
                        continue
                    path = original / member.name
                    assert path.resolve().is_relative_to(original)
                    data = stream.extractfile(member).read()
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(data)
                    record['archive_files'][member.name] = digest(path)
            for name, value in record['source_files'].items():
                destination = owned / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, destination)
                assert digest(destination) == value
            project = owned / registry['project']
            assert registry_identities(project.with_name('Cargo.lock')) <= registry_identities(original / 'Cargo.lock')
            metadata, record['metadata_step'] = run('locked_metadata', ['cargo', 'metadata',
                '--offline', '--locked', '--format-version', '1', '--manifest-path', project])
            record['project_metadata'] = metadata
            _, record['build_step'] = run('locked_build', ['cargo', 'build', '--offline', '--locked',
                '--manifest-path', project, '--bins', '-j2'])
            record['compiled_lock'] = project.with_name('Cargo.lock').read_text()
            for c in contracts:
                corpus = read(ROOT / c['corpus'])
                assert digest(ROOT / c['corpus']) == c['corpus_sha256']
                inputs = [row['input'] for row in corpus['cases']]
                expected = [row['expected'] for row in corpus['cases']]
                payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
                binary = owned / 'target/debug' / c['target']
                output, source_step = run(c['name'] + '_source', [binary], payload)
                assert same(json_lines(output), expected), c['name'] + ': source differs'
                output, native_step = run(c['name'] + '_native', c['native_command'], payload)
                assert same(json_lines(output), expected), c['name'] + ': native differs'
                record['results'].append(dict(name=c['name'], count=c['count'], source_step=source_step,
                    native_step=native_step, source_binary_sha256=digest(binary),
                    native_binary_sha256=digest(Path(c['native_command'][0]))))
                save()
            assert record['source_files'] == {n: digest(owned / n) for n in record['source_files']}
            assert record['archive_files'] == {n: digest(original / n) for n in record['archive_files']}
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = capture_inputs()
        record['references_after'] = references()
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert record['references_before'] == record['references_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        save()
    print('All27 original targets and28 complete corpus/native matrices match; original source bodies preserved.')


if __name__ == '__main__':
    main()
