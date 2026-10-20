#!/usr/bin/env python3
"""Replay complete source-backed stores, build locks and actual Context observations.

Build the six native and optional module consumers first. Retained source
observers use unchanged extraction/bridges; a separate full runtime source view
checks the real Context instead of treating the earlier reduced shell as proof.
"""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

from reference_runtime_view_5_0_6 import ReferenceRuntimeView, digest
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_query_stores_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('query_store', 'query_store_oracle', 259),
    ('id_set_build', 'id_set_build_oracle', 144),
    ('query_store_model', 'query_store_model_5_0_6', 74),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_state_baselines_506'))
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error(f'Record already exists: {args.record}')
    record = {'complete': False, 'steps': [], 'matrices': [], 'full_goal_accepted': False}

    def save():
        if args.record:
            args.record.parent.mkdir(parents=True, exist_ok=True)
            content = (json.dumps(record, indent=2) + '\n').encode()
            args.record.write_bytes(gzip.compress(content, mtime=0)
                                    if args.record.suffix == '.gz' else content)

    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, input_text=None, timeout=1200):
        command = [str(part) for part in command]
        start = time.monotonic()
        result = subprocess.run(command, cwd=ROOT, env=environment, input=input_text,
                                text=True, capture_output=True, timeout=timeout)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'elapsed_seconds': time.monotonic() - start,
                                'stdout': result.stdout, 'stderr': result.stderr})
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        output = run([binary], ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                      for value in inputs), timeout=180)
        if not output.endswith('\n'):
            raise RuntimeError(f'Missing final LF: {binary}')
        values = [json.loads(line) for line in output.split('\n')[:-1]]
        if len(values) != len(inputs):
            raise RuntimeError(f'Incomplete observations: {binary}')
        return values

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    identities = json.loads((PROJECT / 'observers.json').read_text())
    if identities['baseline'] != baseline:
        raise RuntimeError('Store observer manifest baseline differs')
    for name, expected in identities['files'].items():
        if digest(ROOT / name) != expected:
            raise RuntimeError(f'Original observer, extraction or corpus changed: {name}')
    lock = PROJECT / 'Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    if not registry_identities(lock) <= registry_identities(reference_lock):
        raise RuntimeError('Store observer registry versions/checksums differ')
    expected_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name != 'teaql_query_stores_original_source_506':
            if not resolved <= expected_versions.get(name, set()):
                raise RuntimeError(f'Store observer package version differs: {name}')
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    for _, directory, prefix in frontends:
        for stem, _, _ in MATRICES:
            if not (directory / (prefix + stem + '_probe')).is_file():
                raise FileNotFoundError(f'Build the {prefix + stem + "_probe"} target first')
    run(['cargo', 'build', '--offline', '--locked', '--jobs', '2', '--manifest-path',
         PROJECT / 'Cargo.toml', '--bins'])
    record.update(baseline=baseline, reference_lock_sha256=digest(reference_lock),
                  observer_project={name: (PROJECT / name).read_text()
                                    for name in ('Cargo.toml', 'Cargo.lock', 'observers.json')},
                  verifier_sha256=digest(Path(__file__)),
                  source_view_helper_sha256=digest(ROOT / 'scripts/reference_runtime_view_5_0_6.py'))
    context_vectors = []
    for stem, driver, count in MATRICES:
        path = PARITY / (stem + '_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(path.read_bytes()))
        if corpus['baseline'] != baseline or len(corpus['cases']) != count:
            raise RuntimeError(f'Store corpus baseline/geometry differs: {stem}')
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        rust = args.rust_target.resolve() / 'debug' / driver
        original = observe(rust, inputs)
        if not same(original, expected):
            raise RuntimeError(f'Actual original differs from retained observations: {stem}')
        item = {'name': stem, 'cases': count, 'original': original,
                'corpus_sha256': digest(path), 'rust_sha256': digest(rust), 'frontends': {}}
        for label, directory, prefix in frontends:
            binary = directory / (prefix + stem + '_probe')
            actual = observe(binary, inputs)
            if not same(original, actual):
                raise RuntimeError(f'{label} complete store outcomes differ: {stem}')
            item['frontends'][label] = {'binary_sha256': digest(binary), 'outcomes': actual}
            print(f'{stem}: {label} {count}/{count} exact source outcomes', flush=True)
        record['matrices'].append(item)
        if stem == 'query_store':
            context_vectors = [row for row in corpus['cases']
                               if row['input']['mode'] == 'observations']
        save()
    if len(context_vectors) != 36:
        raise RuntimeError('Unexpected retained Context observation geometry')
    with tempfile.TemporaryDirectory(prefix='teaql_query_store_context_source_506_') as directory:
        project = Path(directory)
        view = ReferenceRuntimeView(ROOT, project,
                                    {'src/context/pagination.rs':
                                     PARITY / 'query_stores_context_source_5_0_6.rs'}, run)
        (project / 'inputs.json').write_text(json.dumps([row['input'] for row in context_vectors]))
        environment.update(TEAQL_STORE_CONTEXT_INPUTS=str(project / 'inputs.json'),
                           TEAQL_STORE_CONTEXT_OUTPUTS=str(project / 'outputs.json'))
        output = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2',
                      '--manifest-path', project / 'Cargo.toml', '--lib',
                      'source_context_contracts_5_0_6::', '--', '--nocapture', '--test-threads=1'])
        if '2 passed; 0 failed' not in output:
            raise RuntimeError('Complete Context source contracts did not both pass')
        original = json.loads((project / 'outputs.json').read_text())
        if not same(original, [row['expected'] for row in context_vectors]):
            raise RuntimeError('Actual complete Context differs from retained shell observations')
        record['actual_context'] = {'contracts_passed': 2, 'retained_vectors': 36,
                                    'outputs': original, 'source_view': view.provenance()}
        save()
    record.update(complete=True, cases_per_frontend=477,
                  actual_context_vectors_are_a_subset=True,
                  sources_sha256={name: digest(ROOT / '.reference/teaql-rs' / name) for name in
                                  ('teaql-runtime/src/data_service/cache.rs',
                                   'teaql-runtime/src/context/pagination.rs')})
    save()
    print('Source store/lock/model matrices and actual Context contracts pass; wider callers remain separate.')


if __name__ == '__main__':
    main()
