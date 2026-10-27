#!/usr/bin/env python3
"""Fresh byte-preserved SQL-view replay of the complete executor driver cohort."""
import argparse
import gzip
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from verify_constant_closure_5_0_6 import registry_identities
from reference_sql_view_5_0_6 import ReferenceSqlView


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--contracts', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    registry_path = args.contracts.resolve()
    registry = read(registry_path)
    contracts = registry['contracts']
    assert len(contracts) == 9 and sum(c['count'] for c in contracts) == 8243
    assert type(contracts) is list and contracts
    assert len({c['name'] for c in contracts}) == len(contracts)
    assert all(type(c['count']) is int and c['count'] > 0 for c in contracts)
    names = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z']).split(b'\0')
    paths = {ROOT / os.fsdecode(n) for n in names if n and Path(os.fsdecode(n)).suffix in ('.cpp', '.hpp', '.h', '.inc', '.ixx', '.cppm', '.rs')}
    paths.update([Path(__file__), registry_path, ROOT / 'tests/parity/compare_values.py', ROOT / 'upstream.lock.json',
                  ROOT / 'CMakeLists.txt', ROOT / 'tests/CMakeLists.txt', ROOT / 'CMakePresets.json', ROOT / 'toolchains.lock.json', ROOT / 'scripts/reference_sql_view_5_0_6.py'])
    for c in contracts:
        paths.update(ROOT / c[key] for key in ('corpus', 'driver', 'project'))
        paths.add((ROOT / c['project']).with_name('Cargo.lock'))
        if c['historical_corpus']:
            paths.add(ROOT / c['historical_corpus'])
    hashes = lambda: {str(p): digest(p) for p in sorted(paths)}
    reference = ROOT / '.reference/teaql-rs'
    reference_names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    reference_hashes = {os.fsdecode(n): digest(reference / os.fsdecode(n)) for n in reference_names if n and (
        os.fsdecode(n).endswith('.rs') or Path(os.fsdecode(n)).name in ('Cargo.toml', 'Cargo.lock'))}
    record = dict(complete=False, whole_goal_complete=False, scope=__doc__, registry=registry, steps=[], projects={},
                  contracts=[], input_hashes_before=hashes(), reference_source_files=reference_hashes,
                  native_configuration={str(p): p.read_text() for p in (ROOT / 'CMakeLists.txt', ROOT / 'tests/CMakeLists.txt', ROOT / 'CMakePresets.json')},
                  free_bytes_before=shutil.disk_usage(ROOT).free)
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', PYTHONDONTWRITEBYTECODE='1')

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), compresslevel=1, mtime=0))

    def run(command, payload=None):
        command = [str(v) for v in command]
        result = subprocess.run(command, cwd=ROOT, env=environment, input=payload, text=True, capture_output=True)
        index = len(record['steps'])
        record['steps'].append(dict(command=command, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr,
            stdin_sha256=None if payload is None else hashlib.sha256(payload.encode()).hexdigest(),
            stdin_bytes=None if payload is None else len(payload.encode())))
        save()
        print(' '.join(command[:5]), 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout, index

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        assert registry['runtime_commit'] == read(ROOT / 'upstream.lock.json')['commit']
        assert registry['generator_commit'] == read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
        run(['cargo', '--version'])
        run(['rustc', '-vV'])
        run(['cmake', '--build', '--preset', 'native', '--parallel', '2', '--target',
             *dict.fromkeys(c['native_target'] for c in contracts)])
        with tempfile.TemporaryDirectory(prefix='teaql_sql_executor_driver_replay_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            environment['CARGO_TARGET_DIR'] = str(owned / 'target')
            record['cargo_environment'] = {k: v for k, v in environment.items() if k.startswith('CARGO_')}
            def view_run(command):
                return run(command)[0]

            view = ReferenceSqlView(ROOT, owned / 'sql', ROOT / registry['observer'], view_run)
            record['sql_view'] = view.provenance()
            record['copied_helpers'] = {}
            for name in registry['helpers']:
                source = ROOT / name
                destination = owned / source.relative_to(ROOT / 'tests/parity')
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source, destination)
                assert digest(source) == digest(destination)
                record['copied_helpers'][name] = digest(source)
            for project in sorted({c['project'] for c in contracts}):
                manifest = ROOT / project
                lock = manifest.with_name('Cargo.lock')
                assert registry_identities(lock) <= registry_identities(reference / 'Cargo.lock')
                compiled = owned / 'project'
                compiled.mkdir()

                def absolute_path(match):
                    target = (manifest.parent / match[1]).resolve()
                    if target == ROOT / '.reference/teaql-rs/teaql-sql':
                        target = view.directory
                    return 'path = ' + json.dumps(str(target))

                prepared = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, manifest.read_text())
                compiled_manifest = compiled / 'Cargo.toml'
                compiled_manifest.write_text(prepared)
                shutil.copyfile(lock, compiled / 'Cargo.lock')
                run(['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path', compiled_manifest])
                assert registry_identities(compiled / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
                metadata, metadata_step = run(['cargo', 'metadata', '--offline', '--locked', '--format-version', '1', '--manifest-path', compiled_manifest])
                package = next(p for p in json.loads(metadata)['packages'] if p['manifest_path'] == str(compiled_manifest))
                members = [c for c in contracts if c['project'] == project]
                command = ['cargo', 'build', '--offline', '--locked', '--manifest-path', compiled_manifest, '-j2']
                for c in members:
                    target = next(t for t in package['targets'] if t['name'] == c['target'])
                    assert Path(target['src_path']).resolve() == (ROOT / c['driver']).resolve()
                    command += ['--bin', c['target']]
                _, build_step = run(command)
                record['projects'][project] = dict(manifest=manifest.read_text(), lock=lock.read_text(),
                                                   metadata_step=metadata_step, build_step=build_step, compiled_manifest=prepared, compiled_lock=(compiled / 'Cargo.lock').read_text())
                for c in members:
                    corpus = read(ROOT / c['corpus'])
                    assert corpus['baseline'] == registry['runtime_commit'] and len(corpus['cases']) == c['count']
                    assert digest(ROOT / c['corpus']) == c['corpus_sha256']
                    assert corpus.get('driver', corpus.get('original_driver')) == c['driver'] and corpus.get('driver_sha256', corpus.get('original_driver_sha256')) == c['driver_sha256'] == digest(ROOT / c['driver'])
                    inputs = [row['input'] for row in corpus['cases']]
                    expected = [row['expected'] for row in corpus['cases']]
                    if c['historical_corpus']:
                        historical = ROOT / c['historical_corpus']
                        assert digest(historical) == c['historical_sha256'] == corpus['historical_corpus_sha256']
                        assert same(inputs, [row['input'] for row in read(historical)['cases']])
                    payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
                    original = owned / 'target/debug' / c['target']
                    output, original_step = run([original], payload)
                    observations = json_lines(output)
                    assert len(observations) == c['count'] and same(observations, expected), c['name'] + ': original differs'
                    native = ROOT / 'build_native_current' / c['native_target']
                    output, native_step = run([native], payload)
                    actual = json_lines(output)
                    assert len(actual) == c['count'] and same(actual, observations), c['name'] + ': native differs'
                    record['contracts'].append(dict(name=c['name'], count=c['count'], original_step=original_step,
                        native_step=native_step, original_binary_sha256=digest(original), native_binary_sha256=digest(native)))
                    save()
                    print(c['name'], c['count'], 'fresh source/native match', flush=True)
            record['original_sql_tests'] = []
            for module, count in [('executor::tests', 16), ('executor::diagnostic_tests', 6),
                                  ('executor::mutation_diagnostic_tests', 21), ('diagnostic_stream::tests', 7)]:
                output, index = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                                     view.directory / 'Cargo.toml', '-j2', '--lib', module + '::',
                                     '--', '--nocapture', '--test-threads=1'])
                names = re.findall(r'^test ([^ ]+) [.][.][.]', output, re.M)
                assert len(names) == count and str(count) + ' passed; 0 failed' in output
                record['original_sql_tests'].append(dict(module=module, count=count, tests=names, step=index))
            view.verify()
            record['sql_view'] = view.provenance()
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file() and not p.is_symlink())
        assert reference_hashes == {name: digest(reference / name) for name in reference_hashes}
        record['input_hashes_after'] = hashes()
        assert record['input_hashes_before'] == record['input_hashes_after']
        run(['python3', 'scripts/verify_upstream.py'])
        record['complete'] = True
    except BaseException as error:
        record['error'] = dict(type=type(error).__name__, message=str(error))
        raise
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        record['free_bytes_after'] = shutil.disk_usage(ROOT).free
        save()
    print(len(contracts), 'declared drivers /', sum(c['count'] for c in contracts), 'complete source/native observations match')


if __name__ == '__main__':
    main()
