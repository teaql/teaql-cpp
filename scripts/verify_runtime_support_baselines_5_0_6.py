#!/usr/bin/env python3
"""Reproduce unchanged original purpose diagnostics and DataStore inputs."""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_scalar_baselines_5_0_6 import BASELINE, ROOT, PARITY, digest, read
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    assert json.loads((ROOT / 'upstream.lock.json').read_text())['commit'] == BASELINE
    manifest = PARITY / 'rust_runtime_baselines_5_0_6/Cargo.toml'
    target = Path('/tmp/teaql_cpp_runtime_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(manifest), '--bins'], env=env, check=True)
    staged = []
    summaries = []
    for stem, driver, native in (
            ('purposed_query_debug', 'generated_support', 'teaql_generated_support_probe'),
            ('data_store', 'data_store', 'teaql_data_store_probe')):
        historical = PARITY / f'{stem}_cases.json'
        if not historical.exists():
            historical = historical.with_suffix('.json.gz')
        old = read(historical)
        source = PARITY / f'rust_values/src/bin/{driver}_oracle.rs'
        inputs = [row['input'] for row in old['cases']]
        original = observe(str(target / 'debug' / f'{driver}_oracle'), inputs)
        actual = observe(str(args.build_dir / native), inputs)
        if not same(original, actual):
            raise SystemExit(f'{stem}: original/native differences; record and diagnose before qualification')
        corpus = dict(baseline=BASELINE, historical_sha256=digest(historical),
                      original_driver_sha256=digest(source), manifest_sha256=digest(manifest),
                      lock_sha256=digest(manifest.with_name('Cargo.lock')),
                      cases=[dict(input=value, expected=result) for value, result in zip(inputs, original)])
        active = PARITY / f'{stem}_5_0_6_cases.json.gz'
        if not args.record and not same(read(active), corpus):
            raise SystemExit(f'{stem}: active observations or provenance differ')
        staged.append((active, corpus))
        changed = sum(not same(row['expected'], result) for row, result in zip(old['cases'], original))
        summaries.append(dict(stem=stem, inputs=len(inputs), source_changed=changed,
                              original_driver=str(source.relative_to(ROOT)),
                              original_driver_sha256=digest(source), historical_sha256=digest(historical)))
        print(f'{stem}: {len(inputs)}/{len(inputs)} exact original/native observations', flush=True)
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0))
        (ROOT / 'docs/parity/runtime_support_baselines_5_0_6.json').write_text(
            json.dumps(dict(schema=1, baseline=BASELINE, cases=297, matrices=summaries,
                            normalization='none', scope='Purpose diagnostics and DataStore only; complete capabilities remain partial'), indent=2) + '\n')


if __name__ == '__main__':
    main()
