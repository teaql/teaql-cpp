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

MATRICES = ('runtime_log', 'audit_event', 'context_logging', 'log_formatter', 'log_manager', 'context_event')


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
        if name != 'teaql_logging_baselines_506' and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Observer dependency versions differ from original lock: {name} {resolved}')
    return actual


def observe(executable, inputs):
    result = subprocess.run([str(executable)], text=True, capture_output=True,
                            input=''.join(json.dumps(value, ensure_ascii=True)+'\n' for value in inputs))
    lines = result.stdout.split('\n')
    if lines and not lines[-1]:
        lines.pop()
    if result.returncode or len(lines) != len(inputs):
        path = ROOT / 'docs/parity/logging_baselines_5_0_6_process_failure.json.gz'
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
    manifest = PARITY / 'rust_logging_baselines_5_0_6/Cargo.toml'
    lock = manifest.with_name('Cargo.lock')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    actual_versions = require_original_versions(reference_lock, lock)
    target = Path('/tmp/teaql_cpp_logging_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target), CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(manifest), '--bin', 'runtime_log_oracle', '--bin', 'audit_event_oracle', '--bin', 'context_logging_oracle', '--bin', 'log_formatter_oracle', '--bin', 'log_manager_oracle'], env=env, check=True)
    subprocess.run([sys.executable, str(PARITY / 'build_context_event_oracle_5_0_6.py')], check=True)
    event_provenance = read(ROOT / 'build_rust_context_event_5_0_6/provenance.json')
    helpers = {str(path.relative_to(ROOT)): digest(path) for path in (Path(__file__), PARITY / 'compare_log_manager.py', PARITY / 'build_context_event_oracle_5_0_6.py', PARITY / 'rust_values/src/formatter_fixture.rs', PARITY / 'rust_logging_baselines_5_0_6/build.rs')}
    staged, summaries, differences = [], [], []
    for stem in MATRICES:
        historical = PARITY / f'{stem}_cases.json'
        if not historical.exists():
            historical = historical.with_suffix('.json.gz')
        old = read(historical); old_hash = digest(historical)
        source = PARITY / ('rust_context_event_5_0_6/src/main.rs' if stem == 'context_event' else f'rust_values/src/bin/{stem}_oracle.rs')
        inputs = [case['input'] for case in old['cases']]
        if stem == 'context_event':
            original = observe(Path('/tmp/teaql_cpp_context_event_rust_506/debug/teaql_context_event_506'), inputs)
            actual = observe(args.build_dir / (args.cpp_prefix + 'context_event_probe'), inputs)
        elif stem == 'log_manager':
            from compare_log_manager import observe as manager_observe
            original = manager_observe(target / 'debug/log_manager_oracle', inputs)
            actual = manager_observe(args.build_dir / (args.cpp_prefix + 'log_manager_probe'), inputs)
        elif stem == 'log_formatter':
            probes = {'audit': 'audit_event', 'sql': 'runtime_log', 'config': 'log_formatter'}
            original, actual = [None] * len(inputs), [None] * len(inputs)
            for group, probe in probes.items():
                indexes = [i for i, c in enumerate(old['cases']) if c['probe'] == group]
                values = [inputs[i] for i in indexes]
                rust_values = observe(target / 'debug' / (probe + '_oracle'), values)
                cpp_values = observe(args.build_dir / (args.cpp_prefix + probe + '_probe'), values)
                for i, a, b in zip(indexes, rust_values, cpp_values):
                    original[i], actual[i] = a, b
            assert all(value is not None for value in original + actual)
        else:
            original = observe(target / 'debug' / f'{stem}_oracle', inputs)
            actual = observe(args.build_dir / (args.cpp_prefix + stem + '_probe'), inputs)
        gaps = [dict(matrix=stem, index=i, input=value, rust=a, cpp=b)
                for i, (value, a, b) in enumerate(zip(inputs, original, actual)) if not same(a, b)]
        differences.extend(gaps)
        corpus = dict(baseline=BASELINE, historical_corpus=historical.name, historical_sha256=old_hash,
                      original_driver=str(source.relative_to(ROOT)), original_driver_sha256=digest(source),
                      manifest_sha256=digest(manifest), lock_sha256=digest(lock), reference_lock_sha256=digest(reference_lock),
                      helper_sha256=helpers,
                      **({'complete_context_provenance': event_provenance} if stem == 'context_event' else {}),
                      notice='Historical inputs unchanged; five historical drivers remain byte-identical, while Context event dispatch uses the complete original runtime and recorded access bridges. Source dependencies retain original Cargo.lock versions. JSON outputs exact; existing human-time/header/executable-filename adapters retained explicitly; native64 platform only.',
                      cases=[dict(input=value, expected=output, **({'probe': case['probe']} if 'probe' in case else {})) for case, value, output in zip(old['cases'], inputs, original)])
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
        path = ROOT / 'docs/parity/logging_baselines_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences), differences=differences), indent=2)+'\n').encode(), mtime=0))
        print(json.dumps(differences[0], indent=2)); raise SystemExit(f'{len(differences)} differences; no active corpus written')
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        record = dict(schema=1, baseline=BASELINE, observations=sum(row['observations'] for row in summaries), matrices=summaries,
                      dependency_versions={key:sorted(value) for key,value in actual_versions.items()},
                      manifest_sha256=digest(manifest), lock_sha256=digest(lock), reference_lock_sha256=digest(reference_lock),
                      normalization='JSON output exact; existing formatter adapters remove volatile human time prefix; manager also corrects the documented JSON header claim and canonicalizes executable log filename', status='declared core family qualification only; complete capabilities and integration remain open',
                      complete_capability_acceptance=False)
        (ROOT / 'docs/parity/logging_baselines_5_0_6.json').write_text(json.dumps(record, indent=2)+'\n')
    print('Logging baseline replay complete; historical artifacts preserved')


if __name__ == '__main__':
    main()
