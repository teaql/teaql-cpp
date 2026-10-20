#!/usr/bin/env python3
"""Reexecute historical scalar inputs against locked 5.0.6 and native C++."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same

STEMS = ('decimal', 'decimal_extended', 'decimal_inline', 'decimal_scale',
         'decimal_parsing', 'decimal_numeric', 'decimal_foundation',
         'decimal_wire_format', 'value', 'format')
BASELINE = 'fe82a18982415f1746df9a2cf7c42f5e9abdc6d3'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    with (gzip.open(path, 'rt', encoding='utf-8') if path.suffix == '.gz'
          else path.open(encoding='utf-8')) as source:
        return json.load(source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true',
                        help='Write separate active corpora only after every comparison passes')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--rust-target-dir', type=Path,
                        default=Path('/tmp/teaql_cpp_scalar_baseline_506'))
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    if json.loads((ROOT / 'upstream.lock.json').read_text())['commit'] != BASELINE:
        raise SystemExit('Requalify this checkpoint before changing the source baseline')
    manifest = PARITY / 'rust_scalar_baseline_5_0_6/Cargo.toml'
    lock = manifest.with_name('Cargo.lock')
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target_dir.resolve()),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(manifest), '--bins'], env=env, check=True)
    staged = []
    summaries = []
    for stem in STEMS:
        historical = PARITY / f'{stem}_cases.json'
        if not historical.is_file():
            historical = historical.with_suffix('.json.gz')
        old_hash = digest(historical)
        old = read(historical)
        driver = ('decimal_inline_oracle' if stem == 'decimal_inline' else
                  'value_oracle' if stem in ('value', 'format') else 'decimal_oracle')
        native = ('teaql_decimal_inline_probe' if stem == 'decimal_inline' else
                  'teaql_value_probe' if stem in ('value', 'format') else 'teaql_decimal_probe')
        source = PARITY / 'rust_values/src' / ('main.rs' if driver == 'value_oracle'
                                             else f'bin/{driver}.rs')
        inputs = [case['input'] for case in old['cases']]
        original = observe(str(args.rust_target_dir / 'debug' / driver), inputs)
        actual = observe(str(args.build_dir / native), inputs)
        failed = [i for i, (a, b) in enumerate(zip(original, actual)) if not same(a, b)]
        if failed:
            for index in failed[:10]:
                print(json.dumps({'case': index, 'input': inputs[index],
                                  'original': original[index], 'cpp': actual[index]}))
            raise SystemExit(f'{stem}: {len(failed)}/{len(inputs)} original/native differences')
        active = PARITY / f'{stem}_5_0_6_cases.json.gz'
        corpus = dict(baseline=BASELINE, historical_corpus=historical.name,
                      historical_corpus_sha256=old_hash,
                      original_driver=str(source.relative_to(ROOT)),
                      original_driver_sha256=digest(source),
                      observer_manifest_sha256=digest(manifest),
                      observer_lock_sha256=digest(lock),
                      note='Fresh unchanged driver execution on 5.0.6; historical inputs unchanged; no normalization.',
                      cases=[dict(input=value, expected=result)
                             for value, result in zip(inputs, original)])
        if not args.record and not same(read(active), corpus):
            raise SystemExit(f'{active.name} differs from fresh locked-source observations')
        if digest(historical) != old_hash:
            raise SystemExit(f'Historical evidence changed during execution: {historical}')
        staged.append((active, corpus))
        changed = sum(not same(case['expected'], output)
                      for case, output in zip(old['cases'], original))
        summaries.append(dict(stem=stem, cases=len(inputs), source_changed=changed,
                              native_differences=0, historical_sha256=old_hash,
                              original_driver_sha256=digest(source), active=active.name))
        print(f'{stem}: {len(inputs)}/{len(inputs)} exact original/native observations', flush=True)
    if args.record:
        for path, corpus in staged:
            data = (json.dumps(corpus, ensure_ascii=True, separators=(',', ':')) + '\n').encode()
            path.write_bytes(gzip.compress(data, mtime=0))
        evidence = dict(schema=1, baseline=BASELINE, status='scoped exact source/native qualification',
                        cases=sum(row['cases'] for row in summaries), corpora=summaries,
                        manifest_sha256=digest(manifest), lock_sha256=digest(lock),
                        source='Unchanged historical drivers, frozen original teaql-core and retained exact dependencies',
                        normalization='none', scope='Scalar mechanics only; full capabilities and P0-P7 remain open')
        (ROOT / 'docs/parity/scalar_baselines_5_0_6.json').write_text(
            json.dumps(evidence, indent=2) + '\n')
    print(f'Scalar baselines: {sum(row["cases"] for row in summaries)} exact observations reproduce')


if __name__ == '__main__':
    main()
