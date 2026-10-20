#!/usr/bin/env python3
"""Reexecute byte-preserved SQL driver/input families on the locked 5.0.6 source."""
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
    ('sql_dialect_helper', 'sql_schema_oracle', 'sql_dialect_helper_probe'),
    ('sql_dialect_in', 'sql_schema_oracle', 'sql_dialect_helper_probe'),
    ('sql_projection_scope', 'sql_log_oracle', 'sql_projection_scope_probe'),
    ('sql_transport', 'sql_transport_oracle', 'sql_transport_probe'),
    ('sql_schema', 'sql_schema_oracle', 'sql_schema_probe'),
    ('sql_mutation', 'sql_mutation_oracle', 'sql_mutation_probe'),
    ('sql_dialect_hook', 'sql_mutation_oracle', 'sql_mutation_probe'),
    ('sql_dialect', 'sql_dialect_oracle', 'sql_dialect_probe'),
    ('sql_grouped_partition', 'sql_grouped_partition', 'sql_grouped_partition_probe'),
)


def observe(executable, inputs):
    result = subprocess.run([str(executable)],
                            input=''.join(json.dumps(value, ensure_ascii=True)+'\n' for value in inputs),
                            text=True, capture_output=True)
    lines = result.stdout.split('\n')
    if lines and not lines[-1]:
        lines.pop()
    if result.returncode or len(lines) != len(inputs):
        path = ROOT / 'docs/parity/sql_baselines_5_0_6_process_failure.json.gz'
        if not path.exists():
            failure = dict(executable=str(executable), exit_code=result.returncode,
                           expected_outputs=len(inputs), observed_outputs=len(lines),
                           stdout=result.stdout, stderr=result.stderr)
            path.write_bytes(gzip.compress((json.dumps(failure, indent=2)+'\n').encode(), mtime=0))
        raise RuntimeError(f'{executable}: incomplete/failed observation; see {path}')
    return [json.loads(line) for line in lines]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    parser.add_argument('--rust-target-dir', type=Path, default=Path('/tmp/teaql_cpp_sql_baselines_506'))
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    if read(ROOT / 'upstream.lock.json')['commit'] != BASELINE:
        raise RuntimeError('SQL reference changed; requalification required')
    manifest = PARITY / 'rust_sql_baselines_5_0_6/Cargo.toml'
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target_dir),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    bins = sorted({driver for _, driver, _ in MATRICES})
    command = ['cargo', 'build', '--offline', '--locked', '--manifest-path', str(manifest)]
    for name in bins:
        command += ['--bin', name]
    subprocess.run(command, env=env, check=True)
    helpers = {str(path.relative_to(ROOT)): digest(path) for path in (
        PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'rust_values/src/fixture_sql.rs',
        Path(__file__),
    )}
    staged = []
    matrices = []
    differences = []
    for stem, driver, probe in MATRICES:
        legacy = stem != 'sql_grouped_partition'
        historical = PARITY / (f'{stem}_cases.json' if legacy else 'sql_grouped_partition_inputs_5_0_6.json')
        if not historical.is_file():
            historical = historical.with_suffix('.json.gz')
        old_hash = digest(historical)
        old = read(historical)
        source = PARITY / (f'rust_values/src/bin/{driver}.rs' if legacy else 'rust_sql_baselines_5_0_6/sql_grouped_partition.rs')
        inputs = [case['input'] for case in old['cases']]
        original = observe(args.rust_target_dir / 'debug' / driver, inputs)
        actual = observe(args.build_dir / (args.cpp_prefix + probe), inputs)
        gaps = [dict(matrix=stem, index=i, input=value, rust=a, cpp=b)
                for i, (value, a, b) in enumerate(zip(inputs, original, actual)) if not same(a, b)]
        differences.extend(gaps)
        changed = sum(not same(case['expected'], value) for case, value in zip(old['cases'], original)) if legacy else None
        corpus = dict(baseline=BASELINE, input_fixture=historical.name,
                      input_fixture_sha256=old_hash, historical_inputs=legacy,
                      original_driver=str(source.relative_to(ROOT)), original_driver_sha256=digest(source),
                      observer_manifest_sha256=digest(manifest), observer_lock_sha256=digest(manifest.with_name('Cargo.lock')),
                      observer_helpers_sha256=helpers,
                      notice='Exact locked-source outputs without normalization. Historical drivers/inputs remain unchanged; grouped-partition inputs invoke the original compiler and physical SQLite provider. Native checked build evidence; protocol-incompatible historical execution drivers are separate failures.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, original)])
        active = PARITY / f'{stem}_5_0_6_cases.json.gz'
        if not gaps and not args.record and not same(read(active), corpus):
            raise RuntimeError(f'{stem}: corpus differs from actual locked source')
        if digest(historical) != old_hash:
            raise RuntimeError(f'Historical corpus changed: {historical}')
        staged.append((active, corpus))
        matrices.append(dict(matrix=stem, observations=len(inputs), source_changed=changed,
                             native_differences=len(gaps), original_driver=str(source.relative_to(ROOT)),
                             original_driver_sha256=digest(source), historical_sha256=old_hash,
                             active=str(active.relative_to(ROOT))))
        print(f'{stem}: {len(inputs)-len(gaps)}/{len(inputs)} exact source/native outcomes; source changes {changed}', flush=True)
    if differences:
        path = ROOT / 'docs/parity/sql_baselines_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences), differences=differences), indent=2)+'\n').encode(), mtime=0))
        print(json.dumps(differences[0], indent=2))
        raise SystemExit(f'{len(differences)} SQL differences; no active corpus written')
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        record = dict(schema=1, baseline=BASELINE, observations=sum(row['observations'] for row in matrices),
                      matrices=matrices, manifest_sha256=digest(manifest), lock_sha256=digest(manifest.with_name('Cargo.lock')),
                      observer_helpers_sha256=helpers, normalization='none',
                      status='declared unchanged SQL families requalified; protocol migration and full integration remain open',
                      complete_capability_acceptance=False)
        (ROOT / 'docs/parity/sql_baselines_5_0_6.json').write_text(json.dumps(record, indent=2)+'\n')
    print('SQL source replay complete; historical artifacts preserved')


if __name__ == '__main__':
    main()
