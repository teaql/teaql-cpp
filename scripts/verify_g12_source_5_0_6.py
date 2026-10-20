#!/usr/bin/env python3
"""Replay the five retained G12 assembly/sample families with exact source locks.

Preserve original SDKs, observers and corpora. Build C++ consumers and the
test-only clock library first. This is not full G12/P6 acceptance: R08 and
complete current integration retain their explicit unresolved dispositions.
"""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from reference_runtime_view_5_0_6 import digest
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_g12_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('generated_runtime_assembly', 'generated_runtime_assembly', 72),
    ('generated_sample_state', 'generated_sample_state', 144),
    ('generated_sample_execution', 'generated_sample_execution', 18),
    ('generated_sample_failures', 'generated_sample_execution', 28),
    ('generated_sample_fields', 'generated_sample_fields', 85),
)
MODELS = {'checkers': 'cpp-checker-fixture-core', 'inheritance': 'cpp-inheritance-core',
          'relations': 'cpp-relations-core', 'sample_fields_temporal': 'cpp-sample-temporal-core',
          'sample_fields_runtime': 'cpp-sample-runtime-core'}


def read(path):
    content = gzip.decompress(path.read_bytes()) if path.suffix == '.gz' else path.read_bytes()
    return json.loads(content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_state_baselines_506'))
    parser.add_argument('--record', type=Path)
    parser.add_argument('--original-only', action='store_true',
                        help='Replay Rust only; join separately recorded current C++ gates explicitly')
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error(f'Record already exists: {args.record}')
    record = {'complete': False, 'steps': [], 'matrices': [], 'R08_decision_pending': True,
              'full_G12_qualified': False, 'full_goal_accepted': False}

    def save():
        if args.record:
            args.record.parent.mkdir(parents=True, exist_ok=True)
            content = (json.dumps(record, indent=2) + '\n').encode()
            args.record.write_bytes(gzip.compress(content, mtime=0)
                                    if args.record.suffix == '.gz' else content)

    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, inputs=None, clock=None, millis=None, timeout=1200):
        command = [str(part) for part in command]
        env = environment if clock is None else dict(environment, LD_PRELOAD=str(clock),
                                                     TEAQL_TEST_SAMPLE_WALL_MILLIS=str(millis))
        start = time.monotonic()
        result = subprocess.run(command, cwd=ROOT, env=env, input=inputs,
                                text=True, capture_output=True, timeout=timeout)
        step = {'command': command, 'exit_code': result.returncode,
                'elapsed_seconds': time.monotonic() - start,
                'stdout': result.stdout, 'stderr': result.stderr}
        if clock is not None:
            step.update(clock=str(clock), wall_millis=millis)
        record['steps'].append(step)
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs, clock=None, timeout=180):
        groups = {}
        for index, value in enumerate(inputs):
            groups.setdefault(value['wall_millis'] if clock else None, []).append((index, value))
        output = [None] * len(inputs)
        for millis, rows in groups.items():
            text = run([binary], ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                         for _, value in rows), clock, millis, timeout=timeout)
            if not text.endswith('\n'):
                raise RuntimeError(f'Missing final LF: {binary}')
            values = [json.loads(line) for line in text.split('\n')[:-1]]
            if len(values) != len(rows):
                raise RuntimeError(f'Incomplete G12 observations: {binary}')
            for (index, _), value in zip(rows, values):
                if clock and value['wall_millis'] != millis:
                    raise RuntimeError(f'Actual process clock differs: {binary}')
                output[index] = value
        return output

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    baseline = read(ROOT / 'upstream.lock.json')['commit']
    generator = read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    actual_generator = run(['git', '-C', ROOT / '.reference/teaql_code_gen', 'rev-parse', 'HEAD']).strip()
    if actual_generator != generator or run(['git', '-C', ROOT / '.reference/teaql_code_gen',
                                           'status', '--porcelain']).strip():
        raise RuntimeError('Generator reference is not the clean fixed commit')
    identities = read(PROJECT / 'observers.json')
    if identities['baseline'] != baseline:
        raise RuntimeError('G12 observer manifest baseline differs')
    for name, expected in identities['files'].items():
        if digest(ROOT / name) != expected:
            raise RuntimeError(f'Original G12 observer/helper/corpus changed: {name}')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    if not registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference_lock):
        raise RuntimeError('G12 registry versions/checksums differ')
    reference_versions = versions(reference_lock)
    for name, resolved in versions(PROJECT / 'Cargo.lock').items():
        if name not in {'teaql_g12_current_source_506', *MODELS.values()}:
            if not resolved <= reference_versions.get(name, set()):
                raise RuntimeError(f'G12 package version differs: {name}')
    provenance = {}
    for fixture in MODELS:
        directory = ROOT / 'tests/generator/parity' / (fixture + '_rust')
        data = read(directory / 'PROVENANCE.json')
        if data['hand_edited'] or data['runtime_commit'] != baseline or data['generator_commit'] != generator:
            raise RuntimeError(f'Original generated SDK provenance differs: {fixture}')
        actual = {path.relative_to(directory).as_posix(): digest(path)
                  for path in directory.rglob('*') if path.is_file() and path.name != 'PROVENANCE.json'}
        if actual != data['files'] or digest(ROOT / data['model']) != data['model_sha256']:
            raise RuntimeError(f'Original generated SDK/model bytes differ: {fixture}')
        provenance[fixture] = digest(directory / 'PROVENANCE.json')
    frontends = [] if args.original_only else [
        ('native', args.build_dir.resolve(), 'teaql_', args.build_dir.resolve())]
    if args.module_build_dir and not args.original_only:
        directory = args.module_build_dir.resolve()
        frontends.append(('modules', directory / 'modules', 'teaql_module_', directory))
    for _, directory, prefix, build in frontends:
        if not (build / 'libteaql_sample_fixed_clock.so').is_file():
            raise FileNotFoundError(f'Build teaql_sample_fixed_clock in {build}')
        for _, driver, _ in MATRICES:
            if not (directory / (prefix + driver + '_probe')).is_file():
                raise FileNotFoundError(f'Build {prefix + driver + "_probe"} first')
    source_clock = args.build_dir.resolve() / 'libteaql_sample_fixed_clock.so'
    if not source_clock.is_file():
        raise FileNotFoundError('Build teaql_sample_fixed_clock before the original replay')
    fields = read(PARITY / 'generated_sample_fields_5_0_6_cases.json.gz')
    if fields['clock_fixture_sha256'] != digest(ROOT / 'tests/support/fixed_sample_clock.cpp'):
        raise RuntimeError('Sample clock source provenance differs')
    run(['cargo', 'build', '--offline', '--locked', '--jobs', '2', '--manifest-path',
         PROJECT / 'Cargo.toml', '--bins'])
    record.update(baseline=baseline, generator_commit=generator, model_provenance=provenance,
                  verifier_sha256=digest(Path(__file__)), source_identities=identities,
                  cpp_executed=not args.original_only,
                  project={name: (PROJECT / name).read_text()
                           for name in ('Cargo.toml', 'Cargo.lock', 'observers.json')})
    for stem, driver, count in MATRICES:
        path = PARITY / (stem + '_5_0_6_cases.json.gz')
        if not path.exists():
            path = path.with_suffix('')
        corpus = read(path)
        if corpus['baseline'] != baseline or len(corpus['cases']) != count:
            raise RuntimeError(f'G12 corpus baseline/geometry differs: {stem}')
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        rust = args.rust_target.resolve() / 'debug' / ('g12_' + driver)
        fixed = stem == 'generated_sample_fields'
        original_clock = source_clock if fixed else None
        timeout = 1800 if stem == 'generated_sample_execution' else 180
        original = observe(rust, inputs, original_clock, timeout)
        if not same(expected, original):
            raise RuntimeError(f'Actual original G12 outcomes differ: {stem}')
        item = {'name': stem, 'cases': count, 'corpus_sha256': digest(path),
                'rust_sha256': digest(rust), 'original': original, 'frontends': {}}
        for label, directory, prefix, build in frontends:
            clock = build / 'libteaql_sample_fixed_clock.so' if fixed else None
            binary = directory / (prefix + driver + '_probe')
            actual = observe(binary, inputs, clock, timeout)
            if not same(original, actual):
                raise RuntimeError(f'Complete G12 source outcomes differ: {stem}/{label}')
            item['frontends'][label] = {'binary_sha256': digest(binary), 'outputs': actual}
            if clock:
                item['frontends'][label]['clock_sha256'] = digest(clock)
            print(f'{stem}: {label} {count}/{count} exact source outcomes', flush=True)
        record['matrices'].append(item)
        save()
    record.update(complete=True, source_observations=347,
                  cpp_observations_per_frontend=None if args.original_only else 347)
    save()
    print('Five original G12 families verified. C++ gates are separate in original-only mode; '
          'R08 and complete current integration remain open.')


if __name__ == '__main__':
    main()
