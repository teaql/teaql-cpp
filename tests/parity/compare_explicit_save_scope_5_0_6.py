#!/usr/bin/env python3
"""Compare complete public-save outputs including empty-ledger root scopes."""
import argparse
import gzip
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    source = ROOT / 'docs/parity/explicit_save_scope_valid_original_5_0_6.json.gz'
    original = json.loads(gzip.decompress(source.read_bytes()))
    assert original['complete']
    if args.record and args.record.exists():
        parser.error('Preserve existing candidate receipt')
    result = subprocess.run([str(args.cpp.resolve())], input=''.join(json.dumps(i) + '\n' for i in original['inputs']),
                            text=True, capture_output=True)
    result.check_returncode()
    actual = [json.loads(line) for line in result.stdout.split('\n') if line.strip()]
    expected = original['observations']
    assert len(actual) == len(expected) == len(original['inputs'])
    mismatches = [dict(index=i, input=original['inputs'][i], expected=e, actual=a)
                  for i, (e, a) in enumerate(zip(expected, actual)) if e != a]
    receipt = dict(complete=not mismatches, cases=len(actual), mismatches=mismatches,
                   actual=actual, original=str(source.relative_to(ROOT)), command=[str(args.cpp)],
                   stderr=result.stderr)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print(f'Explicit save scope: {len(actual) - len(mismatches)}/{len(actual)} match')
    return bool(mismatches)


if __name__ == '__main__':
    raise SystemExit(main())
