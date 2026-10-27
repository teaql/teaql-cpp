#!/usr/bin/env python3
"""Compare actual runtime/executor enhancement observations; mismatches fail."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from compare_values import same

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    if args.output and args.output.exists():
        raise RuntimeError('Preserve existing receipts')
    subprocess.run(['python3', 'scripts/verify_upstream.py'], cwd=ROOT, check=True)
    original_path = ROOT/'docs/parity/relation_enhancement_complete_combined_original_5_0_6.json.gz'
    original = json.loads(gzip.decompress(original_path.read_bytes()))
    if not (original['complete'] and original['provenance']['all_original_bodies_preserved']):
        raise AssertionError()
    if not (original['observer_sha256'] == digest(ROOT/'tests/parity/relation_enhancement_complete_source_5_0_6.rs')):
        raise AssertionError()
    for name, expected in original['input_corpora'].items():
        if not (digest(ROOT/'tests/parity'/name) == expected):
            raise AssertionError()
    for name, expected in original['fixture_sha256'].items():
        if not (digest(ROOT/'tests/parity/rust_values/src'/name) == expected):
            raise AssertionError()
    if not (original['provenance']['source_files_sha256']['src/data_service/relation.rs'] == digest(ROOT/'.reference/teaql-rs/teaql-runtime/src/data_service/relation.rs')):
        raise AssertionError()
    inputs = original['inputs']
    expected = [row['result'] for row in original['observations']]
    if not (len(inputs) == len(expected) == 754):
        raise AssertionError()
    if not ([row['case'] for row in original['observations']] == list(range(754))):
        raise AssertionError()
    binary = args.cpp.resolve()
    before = digest(binary)
    result = subprocess.run([*filter(None, args.cpp_emulator.split(';')), str(binary)],
                            input=''.join(json.dumps(row)+'\n' for row in inputs),
                            text=True, capture_output=True, env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
    actual = [json.loads(line) for line in result.stdout.splitlines()] if result.returncode == 0 else []
    if not (digest(binary) == before):
        raise AssertionError()
    differences = [dict(case=index, input=inputs[index], rust=left, cpp=right)
                   for index, (left, right) in enumerate(zip(expected, actual)) if not same(left,right)]
    complete = result.returncode == 0 and len(actual) == len(expected)
    record = dict(complete=complete, parity_passed=complete and not differences,
                  total=len(expected), matched=len(actual)-len(differences), differences=differences,
                  observations=actual, exit_code=result.returncode, stderr=result.stderr,
                  original_receipt_sha256=digest(original_path), executable_sha256=before,
                  controller=Path(__file__).read_text(),
                  source_sha256={name:digest(ROOT/name) for name in [
                      'tests/parity/relation_enhancement_complete_probe.cpp',
                      'teaql_runtime/include/teaql/runtime/detail/data_service/relation_enhancement.hpp']})
    if args.output:
        args.output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    print(f"Complete enhancement: {record['matched']}/{record['total']} match; complete={complete}")
    return not record['parity_passed']


if __name__ == '__main__':
    raise SystemExit(main())
