#!/usr/bin/env python3
"""Re-execute three unchanged generated query-surface drivers against locked Rust 5.0.6."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import traceback

import archive_driver_provenance_5_0_6 as archive
import query_surface_cli_protocol_5_0_6 as protocol
from run_native_integration_epoch_5_0_6 import references

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from expression_cli_cases import require_cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def copied_inputs():
    paths = set(protocol.DRIVERS.values())
    paths.update(protocol.INCLUDED_SOURCES)
    paths.update(protocol.SQL_FIXTURES)
    paths.update(protocol.comparator(name) for name in protocol.NAMES)
    paths.update(str(protocol.corpus(ROOT, name)) for name in protocol.NAMES)
    paths.update(('scripts/query_surface_cli_protocol_5_0_6.py',
                  'scripts/collect_query_surface_cli_original_5_0_6.py',
                  'scripts/archive_driver_provenance_5_0_6.py',
                  'scripts/run_native_integration_epoch_5_0_6.py',
                  'tests/parity/compare_values.py',
                  'tests/parity/expression_cli_cases.py',
                  'upstream.lock.json'))
    for project in protocol.ORIGINAL_PROJECTS:
        paths.add('tests/parity/' + project + '/Cargo.toml')
        paths.add('tests/parity/' + project + '/Cargo.lock')
    generated = []
    baseline = read(ROOT / 'upstream.lock.json')['commit']
    generator = references()['teaql_code_gen']
    for relative, _alias in protocol.FIXTURE_ALIASES:
        provenance = read(ROOT / relative / 'PROVENANCE.json')
        assert provenance['hand_edited'] is False
        assert provenance['runtime_commit'] == baseline
        assert provenance['generator_commit'] == generator
        assert digest(ROOT / provenance['model']) == provenance['model_sha256']
        actual = {path.relative_to(ROOT / relative).as_posix(): digest(path)
                  for path in (ROOT / relative).rglob('*')
                  if path.is_file() and path.name != 'PROVENANCE.json'}
        assert actual == provenance['files']
        paths.update(relative + '/' + name for name in provenance['files'])
        paths.update((relative + '/PROVENANCE.json', provenance['model']))
        generated.append(dict(path=relative, provenance=provenance))
    return sorted(paths), generated


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Preserve historical source evidence')
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    runtime = read(ROOT / 'upstream.lock.json')['commit']
    generator = references()['teaql_code_gen']
    sources, generated = copied_inputs()
    frozen = {name: digest(ROOT / name) for name in sources}
    datasets = []
    for name in protocol.NAMES:
        relative = str(protocol.corpus(ROOT, name))
        corpus = read(ROOT / relative)
        values = protocol.inputs(ROOT, name)
        require_cases(corpus, len(values), values)
        assert corpus['baseline'] == runtime
        datasets.append(dict(name=name, target=name, corpus=relative, selection=None))
    record = dict(
        complete=False, whole_goal_complete=False,
        scope='Three complete unchanged query-surface drivers and three complete generated '
              'domains, using an exact reference-locked source observer. Native comparison '
              'and the remaining custom/protected entries are separate.',
        runtime_commit=runtime, generator_commit=generator,
        source_files=frozen, input_hashes_before=frozen, references_before=references(),
        generated_fixtures=generated, drivers=protocol.DRIVERS,
        dataset_contracts=datasets, steps=[], results=[],
        collector_sha256=digest(Path(__file__)),
        free_bytes_before=shutil.disk_usage(ROOT).free, core_dump_limit=[0, 0],
    )

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))

    def run(label, command, environment, input_text=None):
        command = list(map(str, command))
        print('Starting', label, flush=True)
        started = time.monotonic()
        child = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, text=True, env=environment,
                                 start_new_session=True)
        try:
            stdout, stderr = child.communicate(input_text, timeout=1800)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGTERM)
            stdout, stderr = child.communicate()
            record['steps'].append(dict(label=label, command=command, exit_code=child.returncode,
                                        stdout=stdout, stderr=stderr,
                                        elapsed_seconds=time.monotonic() - started,
                                        timed_out=True))
            save()
            raise RuntimeError(label + ': timed out after child group was reaped')
        record['steps'].append(dict(label=label, command=command, exit_code=child.returncode,
                                    stdout=stdout, stderr=stderr,
                                    elapsed_seconds=time.monotonic() - started))
        save()
        print(label, 'exit', child.returncode, flush=True)
        if child.returncode:
            raise RuntimeError(label + ': ' + stderr[-4000:])
        return stdout

    owned = None
    try:
        assert shutil.disk_usage(ROOT).free > 3 * 1024 ** 3
        data = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'),
                                        'archive', runtime])
        originals = archive.archive_files(data)
        record['archive_sha256'] = hashlib.sha256(data).hexdigest()
        record['archive_files'] = {name: hashlib.sha256(content).hexdigest()
                                   for name, content in originals.items()}
        with tempfile.TemporaryDirectory(prefix='teaql_query_surface_cli_original_506_') as dirname:
            owned = Path(dirname)
            record['owned_directory'] = dirname
            repository = owned / 'repository'
            try:
                for relative, content in originals.items():
                    path = repository / '.reference/teaql-rs' / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(content)
                for relative in sources:
                    path = repository / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes((ROOT / relative).read_bytes())
                project = repository / protocol.PROJECT
                project.mkdir(parents=True)
                manifest = protocol.manifest(repository)
                assert manifest == protocol.manifest(ROOT)
                (project / 'Cargo.toml').write_text(manifest)
                (project / 'Cargo.lock').write_bytes(
                    (repository / '.reference/teaql-rs/Cargo.lock').read_bytes())
                (owned / 'children').mkdir()
                controlled = dict(
                    CARGO_TARGET_DIR=str(owned / 'target'), CARGO_INCREMENTAL='0',
                    CARGO_PROFILE_DEV_DEBUG='0',
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                    TMPDIR=str(owned / 'children'), TEAQL_LOG_ENDPOINT='off',
                )
                environment = dict(os.environ, **controlled)
                record['cargo_environment'] = controlled
                record['derived_observer'] = dict(
                    manifest=manifest,
                    seed_lock_sha256=digest(ROOT / '.reference/teaql-rs/Cargo.lock'),
                )
                run('resolve', ['cargo', 'metadata', '--offline', '--format-version', '1',
                                '--manifest-path', project / 'Cargo.toml'], environment)
                record['derived_observer']['compiled_lock'] = (project / 'Cargo.lock').read_text()
                record['registry_identity_differences'] = sorted(
                    archive.registry_identities(project / 'Cargo.lock') -
                    archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock'))
                assert not record['registry_identity_differences']
                record['derived_observer']['metadata'] = run(
                    'metadata', ['cargo', 'metadata', '--offline', '--locked',
                                 '--format-version', '1', '--manifest-path', project / 'Cargo.toml'],
                    environment)
                record['build_step'] = 2
                raw = run('build', ['cargo', 'build', '--offline', '--locked', '--bins',
                                    '--message-format=json', '--manifest-path',
                                    project / 'Cargo.toml', '-j2'], environment)
                artifacts = {row['target']['name']: row for row in json_lines(raw)
                             if row.get('reason') == 'compiler-artifact'
                             and row.get('executable') is not None}
                assert set(artifacts) == set(protocol.DRIVERS)
                record['compiler_artifacts'] = artifacts
                for dataset in datasets:
                    name = dataset['name']
                    corpus = read(ROOT / dataset['corpus'])
                    values = [row['input'] for row in corpus['cases']]
                    expected = [row['expected'] for row in corpus['cases']]
                    binary = Path(artifacts[name]['executable'])
                    step = len(record['steps'])
                    stream = ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                     for value in values)
                    observed = json_lines(run(name, [binary], environment, input_text=stream))
                    record['results'].append(dict(**dataset,
                                                  corpus_sha256=digest(ROOT / dataset['corpus']),
                                                  inputs=values, expected=expected,
                                                  observed=observed, step=step,
                                                  binary_sha256=digest(binary)))
                    assert same(observed, expected), name
                assert len(record['steps']) == 6 and len(record['results']) == 3
                assert sum(len(row['inputs']) for row in record['results']) == 6764
                assert {name: digest(repository / name) for name in sources} == frozen
                assert {name: digest(repository / '.reference/teaql-rs' / name)
                        for name in originals} == record['archive_files']
            finally:
                record['owned_bytes_removed'] = sum(
                    path.stat().st_size for path in owned.rglob('*')
                    if path.is_file() and not path.is_symlink())
        record['input_hashes_after'] = {name: digest(ROOT / name) for name in sources}
        record['references_after'] = references()
        assert record['input_hashes_after'] == frozen
        assert record['references_after'] == record['references_before']
        record['complete'] = True
    except BaseException:
        record['failure'] = traceback.format_exc()
        raise
    finally:
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        save()
    print('Original query-surface source: 3 complete datasets / 6764 exact outcomes', flush=True)


if __name__ == '__main__':
    main()
