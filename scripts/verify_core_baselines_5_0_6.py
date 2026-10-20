#!/usr/bin/env python3
"""Replay unchanged core/time/presentation/search drivers with reference-lock versions."""
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

MATRICES = ('chrono_value', 'timestamp_api', 'system_time', 'system_time_arithmetic_scope',
            'presentation', 'presentation_api', 'core_graph_api', 'search')


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
        if name != 'teaql_core_baselines_506' and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Observer dependency versions differ from original lock: {name} {resolved}')
    return actual


def observe(executable, inputs):
    result = subprocess.run([str(executable)], text=True, capture_output=True,
                            input=''.join(json.dumps(value, ensure_ascii=True)+'\n' for value in inputs))
    lines = result.stdout.split('\n')
    if lines and not lines[-1]:
        lines.pop()
    if result.returncode or len(lines) != len(inputs):
        path = ROOT / 'docs/parity/core_baselines_5_0_6_process_failure.json.gz'
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
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    assert read(ROOT / 'upstream.lock.json')['commit'] == BASELINE
    manifest = PARITY / 'rust_core_baselines_5_0_6/Cargo.toml'
    lock = manifest.with_name('Cargo.lock')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    actual_versions = require_original_versions(reference_lock, lock)
    target = Path('/tmp/teaql_cpp_core_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target), CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(manifest), '--bins'], env=env, check=True)
    helpers = {str(path.relative_to(ROOT)): digest(path) for path in (Path(__file__), PARITY / 'rust_values/src/fixture_value.rs')}
    staged, summaries, differences = [], [], []
    for stem in MATRICES:
        historical = PARITY / f'{stem}_cases.json'
        if not historical.exists():
            historical = historical.with_suffix('.json.gz')
        old = read(historical); old_hash = digest(historical)
        source = PARITY / f'rust_values/src/bin/{stem}_oracle.rs'
        inputs = [dict(old['setup'], **case['input']) if stem == 'search' else case['input'] for case in old['cases']]
        original = observe(target / 'debug' / f'{stem}_oracle', inputs)
        actual = observe(args.build_dir / (args.cpp_prefix + stem + '_probe'), inputs)
        gaps = [dict(matrix=stem, index=i, input=value, rust=a, cpp=b)
                for i, (value, a, b) in enumerate(zip(inputs, original, actual)) if not same(a, b)]
        differences.extend(gaps)
        corpus = dict(baseline=BASELINE, historical_corpus=historical.name, historical_sha256=old_hash,
                      original_driver=str(source.relative_to(ROOT)), original_driver_sha256=digest(source),
                      manifest_sha256=digest(manifest), lock_sha256=digest(lock), reference_lock_sha256=digest(reference_lock),
                      helper_sha256=helpers,
                      notice='Unchanged historical inputs/driver; source dependencies retain original Cargo.lock versions. Search explicitly combines the unchanged recorded setup with each input. Exact outputs, no normalization; native platform only.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, original)])
        active = PARITY / f'{stem}_baseline_5_0_6_cases.json.gz'
        if not args.record and not gaps and not same(read(active), corpus):
            raise RuntimeError(f'{stem}: recorded corpus differs from actual source')
        assert digest(historical) == old_hash
        staged.append((active, corpus))
        changed = sum(not same(case['expected'], value) for case, value in zip(old['cases'], original))
        summaries.append(dict(matrix=stem, observations=len(inputs), source_changed=changed, native_differences=len(gaps),
                              active=str(active.relative_to(ROOT)), original_driver=str(source.relative_to(ROOT)),
                              original_driver_sha256=digest(source), historical_sha256=old_hash))
        print(f'{stem}: {len(inputs)-len(gaps)}/{len(inputs)} exact original/native outcomes; source changes {changed}', flush=True)
    if differences:
        path = ROOT / 'docs/parity/core_baselines_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences), differences=differences), indent=2)+'\n').encode(), mtime=0))
        print(json.dumps(differences[0], indent=2)); raise SystemExit(f'{len(differences)} differences; no active corpus written')
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        record = dict(schema=1, baseline=BASELINE, observations=sum(row['observations'] for row in summaries), matrices=summaries,
                      dependency_versions={key:sorted(value) for key,value in actual_versions.items()},
                      manifest_sha256=digest(manifest), lock_sha256=digest(lock), reference_lock_sha256=digest(reference_lock),
                      normalization='none', status='declared core family qualification only; complete capabilities and integration remain open',
                      complete_capability_acceptance=False)
        (ROOT / 'docs/parity/core_baselines_5_0_6.json').write_text(json.dumps(record, indent=2)+'\n')
    print('Core baseline replay complete; historical artifacts preserved')


if __name__ == '__main__':
    main()
