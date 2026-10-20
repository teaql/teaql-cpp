#!/usr/bin/env python3
"""Replay unchanged runtime public drivers with reference-lock versions."""
import argparse
import gzip
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_scalar_baselines_5_0_6 import BASELINE, ROOT, PARITY, digest, read
from compare_values import same

MATRICES = ('runtime_id_model', 'runtime_error_debug', 'unified_buffer_clone', 'locking', 'inmemory', 'entity_reference', 'round_trip_reference', 'decimal_pointer_scope', 'entity_query_boundary')
DRIVERS = {'runtime_error_debug': 'runtime_check', 'unified_buffer_clone': 'log_model', 'decimal_pointer_scope': 'decimal_pointer', 'entity_query_boundary': 'entity_query_current'}
PROBES = {'runtime_error_debug': 'runtime_check', 'decimal_pointer_scope': 'decimal', 'entity_query_boundary': 'entity_query'}


def versions(path):
    result = {}
    for package in path.read_text().split('[[package]]')[1:]:
        name = re.search(r'^name = "([^"]+)"', package, re.M)
        version = re.search(r'^version = "([^"]+)"', package, re.M)
        if name and version:
            result.setdefault(name[1], set()).add(version[1])
    return result


def require_original_versions(reference_lock, observer_lock):
    expected = versions(reference_lock)
    actual = versions(observer_lock)
    for name, resolved in actual.items():
        if name != 'teaql_final_baselines_506' and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Observer dependency versions differ from original lock: {name} {resolved}')
    return actual


def observe(executable, inputs):
    result = subprocess.run([str(executable)], text=True, capture_output=True,
                            input=''.join(json.dumps(value, ensure_ascii=True)+'\n' for value in inputs))
    lines = result.stdout.split('\n')
    if lines and not lines[-1]:
        lines.pop()
    if result.returncode or len(lines) != len(inputs):
        path = ROOT / 'docs/parity/final_baselines_5_0_6_process_failure.json.gz'
        if not path.exists():
            failure = dict(executable=str(executable), exit_code=result.returncode,
                           expected=len(inputs), observed=len(lines), stdout=result.stdout, stderr=result.stderr)
            path.write_bytes(gzip.compress((json.dumps(failure, indent=2)+'\n').encode(), mtime=0))
        raise RuntimeError(f'Incomplete/failed observer: {executable}; see {path}')
    return [json.loads(line) for line in lines]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    parser.add_argument('--release-build-dir', type=Path, default=Path('/tmp/teaql_cpp_sqlite_release_506'))
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    assert read(ROOT / 'upstream.lock.json')['commit'] == BASELINE
    manifest = PARITY / 'rust_final_baselines_5_0_6/Cargo.toml'
    lock = manifest.with_name('Cargo.lock')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    actual_versions = require_original_versions(reference_lock, lock)
    target = Path('/tmp/teaql_cpp_final_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target), CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(manifest), '--bins'], env=env, check=True)
    helpers = {str(path.relative_to(ROOT)): digest(path) for path in (Path(__file__), PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'rust_values/src/formatter_fixture.rs')}
    staged, summaries, differences = [], [], []
    for stem in MATRICES:
        historical = PARITY / f'{stem}_cases.json'
        if not historical.exists():
            historical = historical.with_suffix('.json.gz')
        old = read(historical); old_hash = digest(historical)
        driver = DRIVERS.get(stem, stem)
        source = PARITY / ('rust_decimal_pointer/src/main.rs' if stem == 'decimal_pointer_scope' else 'rust_private_save_5_0_6/src/bin/entity_query_5_0_6.rs' if stem == 'entity_query_boundary' else f'rust_values/src/bin/{driver}_oracle.rs')
        inputs = [case['input'] for case in old['cases']]
        if stem == 'entity_query_boundary':
            inputs += read(PARITY / 'entity_query_boundary_controls_5_0_6.json')
        binary = driver if stem == 'entity_query_boundary' else driver + '_oracle'
        original = observe(target / 'debug' / binary, inputs)
        actual = observe(args.build_dir / (args.cpp_prefix + PROBES.get(stem, stem) + '_probe'), inputs)
        release_outputs = None
        if stem in ('runtime_id_model', 'entity_query_boundary'):
            subprocess.run(['cargo', 'build', '--offline', '--locked', '--release',
                            '--manifest-path', str(manifest), '--bin', binary], env=env, check=True)
            release_outputs = observe(target / 'release' / binary, inputs)
            release_cpp = observe(args.release_build_dir / ('teaql_' + PROBES.get(stem, stem) + '_probe'), inputs)
            release_gaps = [dict(matrix=stem, profile='release', index=i, input=v, rust=a, cpp=b)
                            for i, (v, a, b) in enumerate(zip(inputs, release_outputs, release_cpp)) if not same(a, b)]
            differences.extend(release_gaps)
            print(f'{stem}: Release {len(inputs)-len(release_gaps)}/{len(inputs)} exact outcomes', flush=True)
        gaps = [dict(matrix=stem, index=i, input=value, rust=a, cpp=b)
                for i, (value, a, b) in enumerate(zip(inputs, original, actual)) if not same(a, b)]
        differences.extend(gaps)
        corpus = dict(baseline=BASELINE, historical_corpus=historical.name, historical_sha256=old_hash,
                      original_driver=str(source.relative_to(ROOT)), original_driver_sha256=digest(source),
                      manifest_sha256=digest(manifest), lock_sha256=digest(lock), reference_lock_sha256=digest(reference_lock),
                      helper_sha256=helpers,
                      notice='Historical inputs unchanged; current public query fixture retains explicit constructor behavior and separately declared intent controls; other historical drivers unchanged; source dependencies retain original Cargo.lock versions. Exact outputs, no normalization; native64 Debug, with separately replayed native64 Release for runtime ID and query boundaries. Historical 32-bit profiles remain separate.',
                      pointer_bits=64,
                      **({'controls_sha256': digest(PARITY / 'entity_query_boundary_controls_5_0_6.json')} if stem == 'entity_query_boundary' else {}),
                      cases=[dict(input=value, expected={'64': output} if stem == 'decimal_pointer_scope' else output, **({'expected_release': release_outputs[i]} if release_outputs is not None else {})) for i, (value, output) in enumerate(zip(inputs, original))])
        active = PARITY / f'{stem}_baseline_5_0_6_cases.json.gz'
        if not args.record and not gaps and not same(read(active), corpus):
            raise RuntimeError(f'{stem}: recorded corpus differs from actual source')
        assert digest(historical) == old_hash
        staged.append((active, corpus))
        changed = sum(not same(case['expected']['64'] if stem == 'decimal_pointer_scope' else case['expected'], value) for case, value in zip(old['cases'], original))
        summaries.append(dict(matrix=stem, observations=len(inputs), release_observations=len(inputs) if release_outputs is not None else 0, source_changed=changed, historical_inputs=len(old['cases']), native_differences=len(gaps),
                              active=str(active.relative_to(ROOT)), original_driver=str(source.relative_to(ROOT)),
                              original_driver_sha256=digest(source), historical_sha256=old_hash))
        print(f'{stem}: {len(inputs)-len(gaps)}/{len(inputs)} exact original/native outcomes; source changes {changed}', flush=True)
    if differences:
        path = ROOT / 'docs/parity/final_baselines_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences), differences=differences), indent=2)+'\n').encode(), mtime=0))
        print(json.dumps(differences[0], indent=2)); raise SystemExit(f'{len(differences)} differences; no active corpus written')
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        record = dict(schema=1, baseline=BASELINE, observations=sum(row['observations'] for row in summaries), release_observations=sum(row['release_observations'] for row in summaries), matrices=summaries,
                      dependency_versions={key:sorted(value) for key,value in actual_versions.items()},
                      manifest_sha256=digest(manifest), lock_sha256=digest(lock), reference_lock_sha256=digest(reference_lock),
                      normalization='none', status='declared core family qualification only; complete capabilities and integration remain open',
                      complete_capability_acceptance=False)
        (ROOT / 'docs/parity/final_baselines_5_0_6.json').write_text(json.dumps(record, indent=2)+'\n')
    print('Remaining baseline replay complete; historical artifacts preserved')


if __name__ == '__main__':
    main()
