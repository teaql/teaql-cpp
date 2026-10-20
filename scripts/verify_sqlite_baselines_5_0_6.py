#!/usr/bin/env python3
"""Reexecute historical SQLite inputs against locked 5.0.6 without relabeling evidence."""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_scalar_baselines_5_0_6 import BASELINE, ROOT, PARITY, digest, read
from compare_values import same

MATRICES = (
    ('sqlite_schema', 'sqlite_schema_oracle', 'teaql_sqlite_schema_probe', ()),
    ('sqlite_decoding', 'sqlite_decoding_oracle', 'teaql_sqlite_decoding_checked_probe', ()),
    ('sqlite_bindings', 'sqlite_bindings_oracle', 'teaql_sqlite_bindings_probe', ()),
    ('sqlite_error_model', 'sqlite_error_model_oracle', 'teaql_sqlite_error_model_probe', ()),
    ('sqlite_native_diagnostics', 'sqlite_native_diagnostics_oracle', 'teaql_sqlite_native_diagnostics_probe', ()),
    ('sqlite_simple_error', 'sqlite_simple_error_oracle', 'teaql_sqlite_simple_error_probe', ()),
    ('sqlite_from_sql_error', 'sqlite_from_sql_error_oracle', 'teaql_sqlite_from_sql_error_probe', ()),
    ('sqlite_from_sql_raw_debug', 'sqlite_from_sql_error_oracle', 'teaql_sqlite_from_sql_error_probe', ('--raw-debug',)),
)


def timestamp_controls():
    base = '2024-02-29 01:02:03'
    texts = [base, '2024-02-30 01:-2:03', '2024-02-29 xx:-2:03']
    for index in (0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18):
        for character in ('/', ':', 'x'):
            texts.append(base[:index] + character + base[index + 1:])
    texts += [base + suffix for suffix in (
        '+0/', '+x/', '+/x', '+x0', '+0x', '+24:0/', '+0x:0/', '+24:00',
        '+23:60', '-01:00', '+08:', '+08:00x', '.', '.x', '.1234567890+00:00')]
    texts += ['2024-02-30 01:02:03+x/', '2024-02-29 24:02:03+x/',
              '2024-02-29 01:02:60+x/', '2024-02-29 01:02:03.0+x/']
    return [dict(decl='TIMESTAMP', sql_value="CAST(x'" + text.encode().hex() + "' AS TEXT)")
            for text in texts]


def observe(executable, inputs, arguments=()):
    result = subprocess.run([str(executable), *arguments],
                            input=''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs),
                            text=True, capture_output=True)
    if result.returncode:
        failure = dict(executable=str(executable), exit_code=result.returncode,
                       stdout=result.stdout, stderr=result.stderr)
        path = ROOT / 'docs/parity/sqlite_baselines_5_0_6_process_failure.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(failure, indent=2)+'\n').encode(), mtime=0))
        raise RuntimeError(f'{executable}: exit {result.returncode}; retained process failure in {path}')
    lines = result.stdout.split('\n')
    if lines and not lines[-1]:
        lines.pop()
    values = [json.loads(line) for line in lines]
    if len(values) != len(inputs):
        raise RuntimeError(f'{executable}: incomplete output {len(values)}/{len(inputs)}')
    return values, result.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--rust-target-dir', type=Path, default=Path('/tmp/teaql_cpp_sqlite_baselines_506'))
    parser.add_argument('--profile', choices=('checked', 'unchecked'), default='checked')
    parser.add_argument('--cpp-prefix', default='teaql_')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    if json.loads((ROOT / 'upstream.lock.json').read_text())['commit'] != BASELINE:
        raise RuntimeError('SQLite baseline reference changed')
    manifest = PARITY / 'rust_sqlite_baselines_5_0_6/Cargo.toml'
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target_dir),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    decoder = args.build_dir / (args.cpp_prefix + 'sqlite_decoding_checked_probe')
    native_profile = subprocess.run([str(decoder), '--profile'], input='', text=True,
                                    capture_output=True, check=True).stdout.strip()
    if native_profile != args.profile:
        raise RuntimeError(f'Decoder profile mismatch: source {args.profile}, native {native_profile}')
    build_flags = ['--release'] if args.profile == 'unchecked' else []
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(manifest), '--bins', *build_flags],
                   env=env, check=True)
    staged = []
    summaries = []
    differences = []
    diagnostics = []
    helpers = {path.relative_to(ROOT).as_posix(): digest(path) for path in (
        PARITY / 'rust_values/src/fixture_value.rs',
        PARITY / 'rust_values/src/fixture_sql.rs',
        manifest.with_name('sqlite_decoding_checked.rs'),
        PARITY / 'sqlite_decoding_checked_probe.cpp',
        PARITY / 'sqlite_decoding_probe.cpp',
        Path(__file__),
    )}
    for stem, driver, native, arguments in MATRICES:
        historical = PARITY / f'{stem}_cases.json'
        if not historical.is_file():
            historical = historical.with_suffix('.json.gz')
        old = read(historical)
        old_hash = digest(historical)
        source = PARITY / f'rust_values/src/bin/{driver}.rs'
        inputs = [case['input'] for case in old['cases']]
        rust_profile = 'release' if args.profile == 'unchecked' else 'debug'
        original, source_stderr = observe(args.rust_target_dir / rust_profile / driver, inputs, arguments)
        actual, native_stderr = observe(args.build_dir / native.replace('teaql_', args.cpp_prefix, 1), inputs, arguments)
        diagnostics.append(dict(matrix=stem, source_stderr=source_stderr, native_stderr=native_stderr))
        gaps = [dict(matrix=stem, index=i, input=input, rust=a, cpp=b)
                for i, (input, a, b) in enumerate(zip(inputs, original, actual)) if not same(a, b)]
        differences.extend(gaps)
        changed = sum(not same(case['expected'], value) for case, value in zip(old['cases'], original))
        corpus = dict(baseline=BASELINE, checked_arithmetic=args.profile == 'checked', historical_corpus=historical.name,
                      historical_corpus_sha256=old_hash, original_driver=source.relative_to(ROOT).as_posix(),
                      original_driver_sha256=digest(source), observer_manifest_sha256=digest(manifest),
                      observer_lock_sha256=digest(manifest.with_name('Cargo.lock')),
                      observer_helpers_sha256=helpers,
                      notice='Unchanged historical driver and inputs on actual locked 5.0.6 source. Exact ordinary outputs, no normalization. Decoding wrappers report only the exact checked subtraction panic/exception; raw panic diagnostics retained separately. Native64 raw debug remains width-scoped.',
                      cases=[dict(input=input, expected=value) for input, value in zip(inputs, original)])
        qualifier = '_unchecked' if args.profile == 'unchecked' else ''
        active = PARITY / f'{stem}{qualifier}_5_0_6_cases.json.gz'
        if not gaps and not args.record and not same(read(active), corpus):
            raise RuntimeError(f'{stem}: active corpus differs from actual source')
        if digest(historical) != old_hash:
            raise RuntimeError(f'Historical corpus was modified: {stem}')
        staged.append((active, corpus))
        summaries.append(dict(matrix=stem, observations=len(inputs), source_changed=changed,
                              checked_failures=sum(value.get('checked_failure') == 'arithmetic_overflow' for value in original),
                              native_differences=len(gaps), driver=source.relative_to(ROOT).as_posix(),
                              driver_sha256=digest(source), historical_sha256=old_hash,
                              active=active.relative_to(ROOT).as_posix()))
        print(f'{stem}: {len(inputs)-len(gaps)}/{len(inputs)} exact original/native outcomes; changed source outputs {changed}', flush=True)
    controls = timestamp_controls()
    control_original, control_stderr = observe(args.rust_target_dir / rust_profile / 'sqlite_decoding_oracle', controls)
    control_actual, native_stderr = observe(decoder, controls)
    differences.extend(dict(matrix='timestamp_short_circuit', index=i, input=value, rust=a, cpp=b)
                       for i, (value, a, b) in enumerate(zip(controls, control_original, control_actual)) if not same(a, b))
    diagnostics.append(dict(matrix='timestamp_short_circuit', source_stderr=control_stderr, native_stderr=native_stderr))
    print(f'Timestamp short-circuit controls: {len(controls)} actual source/native comparisons', flush=True)
    control_cases = [dict(input=value, expected=output) for value, output in zip(controls, control_original)]
    if not args.record:
        qualifier = '_unchecked' if args.profile == 'unchecked' else ''
        recorded = read(ROOT / f'docs/parity/sqlite_baselines{qualifier}_5_0_6.json')
        if not same(recorded['timestamp_short_circuit_controls'], control_cases):
            raise RuntimeError('Recorded timestamp controls differ from actual locked source')
    if differences:
        path = ROOT / 'docs/parity/sqlite_baselines_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences), differences=differences), indent=2)+'\n').encode(), mtime=0))
        print(json.dumps(differences[0], indent=2))
        raise SystemExit(f'{len(differences)} original/native SQLite differences; no corpus activated')
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        record = dict(schema=1, baseline=BASELINE, checked_arithmetic=args.profile == 'checked', status='declared SQLite corpus families requalified; full integration remains pending',
                      observations=sum(row['observations'] for row in summaries), matrices=summaries,
                      manifest_sha256=digest(manifest), lock_sha256=digest(manifest.with_name('Cargo.lock')),
                      observer_helpers_sha256=helpers,
                      timestamp_short_circuit_controls=control_cases,
                      dependency_scope='Exact direct dependencies from the original 5.0.6 lock, including rusqlite 0.40.1 and libsqlite3-sys 0.38.1. Development observer only.',
                      normalization='none', complete_capability_acceptance=False)
        (ROOT / f'docs/parity/sqlite_baselines{qualifier}_5_0_6.json').write_text(json.dumps(record, indent=2)+'\n')
        (ROOT / f'docs/parity/sqlite_baselines{qualifier}_5_0_6_runtime_diagnostics.json.gz').write_bytes(
            gzip.compress((json.dumps(diagnostics, indent=2)+'\n').encode(), mtime=0))
        if args.profile == 'checked':
            first = next(case for _, corpus in staged for case in corpus['cases']
                         if case['expected'].get('checked_failure') == 'arithmetic_overflow')
            unwrapped = subprocess.run([str(args.rust_target_dir / 'debug/sqlite_decoding_unwrapped_oracle')],
                                       input=json.dumps(first['input'])+'\n', text=True, capture_output=True)
            if unwrapped.returncode == 0 or 'attempt to subtract with overflow' not in unwrapped.stderr:
                raise RuntimeError('The unchanged decoding driver no longer demonstrates the checked source panic')
            (ROOT / 'docs/parity/sqlite_timestamp_checked_source_failure_5_0_6.json').write_text(
                json.dumps(dict(input=first['input'], wrapper_expected=first['expected'],
                                exit_code=unwrapped.returncode, stdout=unwrapped.stdout,
                                stderr=unwrapped.stderr, baseline=BASELINE), indent=2)+'\n')
    print('SQLite baseline replay complete; historical artifacts preserved')


if __name__ == '__main__':
    main()
