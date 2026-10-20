#!/usr/bin/env python3
"""Replay retained SQLite transaction outcomes against the locked Rust source.

Build the C++ probes first. This development-only oracle project shares the
unchanged observers and verifies dependency identities before compiling Rust.
The complete native transaction-stream lifecycle test is a separate CTest gate.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'tests/parity/rust_sqlite_transaction_reference_5_0_6'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same

MATRICES = (
    ('sqlite_async_lease', 'sqlite_async_lease_5_0_6_cases.json', 144),
    ('sqlite_async_schedule', 'sqlite_async_schedule_5_0_6_cases.json.gz', 183),
    ('sqlite_transport', 'sqlite_sync_transaction_5_0_6_cases.json.gz', 183),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    content = gzip.decompress(path.read_bytes()) if path.suffix == '.gz' else path.read_bytes()
    return json.loads(content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path,
                        help='Also replay imported probes in this configured build directory')
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_state_baselines_506'))
    parser.add_argument('--record', type=Path, help='Write new evidence; never overwrite a prior record')
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

    def run(command, input_text=None, timeout=300):
        command = [str(part) for part in command]
        started = time.monotonic()
        result = subprocess.run(command, cwd=ROOT, env=environment, input=input_text,
                                text=True, capture_output=True, timeout=timeout)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'elapsed_seconds': time.monotonic() - started,
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
            raise RuntimeError(f'Incomplete output: {binary}: {len(values)}/{len(inputs)}')
        return values

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    identities = load(PROJECT / 'observers.json')
    if identities['baseline'] != baseline:
        raise RuntimeError('Observer manifest belongs to another baseline')
    for name, expected in identities['files'].items():
        if digest(ROOT / name) != expected:
            raise RuntimeError(f'Original observer or retained corpus changed: {name}')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    observer_lock = PROJECT / 'Cargo.lock'
    if not registry_identities(observer_lock) <= registry_identities(reference_lock):
        raise RuntimeError('Oracle registry versions/checksums differ from the reference')
    reference_versions = versions(reference_lock)
    for name, resolved in versions(observer_lock).items():
        if name != 'teaql_sqlite_transaction_streaming_reference_506':
            if not resolved <= reference_versions.get(name, set()):
                raise RuntimeError(f'Oracle package version differs: {name}')
    record.update(baseline=baseline, reference_lock_sha256=digest(reference_lock),
                  observer_lock_sha256=digest(observer_lock),
                  observer_manifest=identities, verifier_sha256=digest(Path(__file__)))
    save()
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    for _, directory, prefix in frontends:
        for stem, _, _ in MATRICES:
            if not (directory / (prefix + stem + '_probe')).is_file():
                raise FileNotFoundError(f'Build the probe first: {directory / (prefix + stem + "_probe")}')
    run(['cargo', 'build', '--offline', '--locked', '--jobs', '1', '--manifest-path',
         PROJECT / 'Cargo.toml', '--bins'], timeout=1200)
    for stem, corpus_name, count in MATRICES:
        corpus_path = ROOT / 'tests/parity' / corpus_name
        corpus = load(corpus_path)
        if corpus['baseline'] != baseline or len(corpus['cases']) != count:
            raise RuntimeError(f'Unexpected corpus baseline or geometry: {corpus_name}')
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        rust = args.rust_target.resolve() / 'debug' / ('streaming_reference_' + stem)
        original = observe(rust, inputs)
        if not same(original, expected):
            raise RuntimeError(f'Actual locked Rust outcomes differ from the corpus: {stem}')
        item = {'name': stem, 'cases': count, 'corpus_sha256': digest(corpus_path),
                'rust_sha256': digest(rust), 'original': original, 'frontends': {}}
        for label, directory, prefix in frontends:
            binary = directory / (prefix + stem + '_probe')
            actual = observe(binary, inputs)
            if not same(original, actual):
                mismatch = next(i for i, pair in enumerate(zip(original, actual))
                                if not same(*pair))
                raise RuntimeError(f'{label} differs: {stem} case {mismatch}: '
                                   f'{inputs[mismatch]!r}: {original[mismatch]!r} != {actual[mismatch]!r}')
            item['frontends'][label] = {'binary_sha256': digest(binary), 'outcomes': actual}
            print(f'{stem}: {label} {count}/{count} exact source outcomes', flush=True)
        record['matrices'].append(item)
        save()
    record.update(complete=True, cases_per_frontend=sum(row[2] for row in MATRICES),
                  frontends=[row[0] for row in frontends])
    save()
    print('Retained transaction outcomes verified. Stream lifecycle acceptance remains a separate gate.')


if __name__ == '__main__':
    main()
