#!/usr/bin/env python3
"""Run complete original Language/I18n tests and retained source observations.

Build the C++ probes and lifetime contract before this development-only replay.
The unit wrapper includes the actual unchanged source modules and JSON fixtures;
the two existing observers execute the frozen runtime with its dependency lock.
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
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_i18n_source_5_0_6'
OBSERVERS = PARITY / 'rust_state_baselines_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


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

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    identities = json.loads((PROJECT / 'observers.json').read_text())
    if identities['baseline'] != baseline:
        raise RuntimeError('I18n observer manifest baseline differs')
    for name, expected in identities['files'].items():
        if digest(ROOT / name) != expected:
            raise RuntimeError(f'Original I18n source, observer or corpus changed: {name}')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for project, local in [(PROJECT, 'teaql_i18n_source_units_506'),
                           (OBSERVERS, 'teaql_state_baselines_506')]:
        lock = project / 'Cargo.lock'
        if not registry_identities(lock) <= registry_identities(reference_lock):
            raise RuntimeError(f'I18n registry versions/checksums differ: {project.name}')
        for name, resolved in versions(lock).items():
            if name != local and not resolved <= reference_versions.get(name, set()):
                raise RuntimeError(f'I18n package version differs: {name}')
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    for _, directory, prefix in frontends:
        for stem in ('i18n', 'i18n_debug_scope'):
            if not (directory / (prefix + stem + '_probe')).is_file():
                raise FileNotFoundError(f'Build the {prefix + stem + "_probe"} target first')
    unit_output = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2',
                       '--manifest-path', PROJECT / 'Cargo.toml', '--lib'])
    if '4 passed; 0 failed' not in unit_output:
        raise RuntimeError('The complete original Language/I18n unit tests did not all pass')
    run(['cargo', 'build', '--offline', '--locked', '--jobs', '2', '--manifest-path',
         OBSERVERS / 'Cargo.toml', '--bin', 'i18n_oracle', '--bin', 'i18n_debug_scope_oracle'])
    record.update(baseline=baseline, source_identities=identities,
                  unit_wrapper={name: (PROJECT / name).read_text()
                                for name in ('Cargo.toml', 'Cargo.lock', 'lib.rs')},
                  observer_project_sha256={name: digest(OBSERVERS / name)
                                           for name in ('Cargo.toml', 'Cargo.lock', 'build.rs')},
                  original_unit_tests=4, verifier_sha256=digest(Path(__file__)))
    for stem, count in [('i18n', 4575), ('i18n_debug_scope', 60)]:
        corpus_path = PARITY / (stem + '_baseline_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
        if corpus['baseline'] != baseline or len(corpus['cases']) != count:
            raise RuntimeError(f'I18n corpus baseline/geometry differs: {stem}')
        inputs = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n'
                         for row in corpus['cases'])
        expected = [row['expected'] for row in corpus['cases']]
        binaries = [('original', args.rust_target.resolve() / 'debug' / (stem + '_oracle'))]
        binaries += [(label, directory / (prefix + stem + '_probe'))
                     for label, directory, prefix in frontends]
        item = {'name': stem, 'cases': count, 'corpus_sha256': digest(corpus_path),
                'observations': {}}
        for label, binary in binaries:
            output = run([binary], inputs, timeout=180)
            if not output.endswith('\n'):
                raise RuntimeError(f'Missing final LF: {binary}')
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            if not same(expected, actual):
                raise RuntimeError(f'Complete I18n outcomes differ: {stem}/{label}')
            item['observations'][label] = {'binary_sha256': digest(binary), 'outputs': actual}
            print(f'{stem}: {label} {count}/{count} exact source outcomes', flush=True)
        record['matrices'].append(item)
        save()
    record.update(complete=True, cases_per_frontend=4635)
    save()
    print('Complete original tests and I18n observations pass; whole Context/Checker acceptance remains separate.')


if __name__ == '__main__':
    main()
