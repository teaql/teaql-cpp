#!/usr/bin/env python3
"""Compare private memory entry points; mismatches are evidence, never passing parity."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import same

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True, type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    if args.output and args.output.exists():
        raise RuntimeError('Do not overwrite retained observations')
    subprocess.run(['python3', 'scripts/verify_upstream.py'], cwd=ROOT, check=True)
    original_path = ROOT / 'docs/parity/memory_entries_original_5_0_6.json.gz'
    original = json.loads(gzip.decompress(original_path.read_bytes()))
    observer = ROOT / 'tests/parity/memory_entries_source_5_0_6.rs'
    source = ROOT / '.reference/teaql-rs/teaql-runtime/src/memory.rs'
    if (not original['complete']
            or not original['provenance']['all_original_bodies_preserved']
            or original['observer_sha256'] != digest(observer)
            or original['provenance']['source_files_sha256']['src/memory.rs'] != digest(source)):
        raise RuntimeError('Original observation provenance does not match current inputs')
    expected = original['observations']
    executable = args.cpp.resolve()
    before = digest(executable)
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    result = subprocess.run([*emulator, str(executable)], capture_output=True, text=True, check=True)
    actual = [json.loads(line) for line in result.stdout.splitlines()]
    if len(expected) != 43 or len(actual) != len(expected):
        raise RuntimeError('Incomplete observation set')
    if [row['case'] for row in expected] != [row['case'] for row in actual]:
        raise RuntimeError('Mismatched case identity/order')
    if digest(executable) != before:
        raise RuntimeError('Executable changed during observation')
    differences = [{'case': left['case'], 'rust': left, 'cpp': right}
                   for left, right in zip(expected, actual) if not same(left, right)]
    paths = ['tests/parity/memory_entries_probe.cpp',
             'tests/parity/memory_data_service.hpp',
             'tests/parity/compare_values.py', 'tests/CMakeLists.txt']
    record = dict(complete=True, parity_passed=not differences,
                  baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                  original_receipt_sha256=digest(original_path),
                  controller=Path(__file__).read_text(),
                  source_sha256={path: digest(ROOT/path) for path in paths},
                  executable_sha256=before, observations=actual,
                  differences=differences, stderr=result.stderr,
                  matched=len(expected)-len(differences), total=len(expected))
    if args.output:
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))
    print(f"Private memory entries: {record['matched']}/{record['total']} match")
    return bool(differences)


if __name__ == '__main__':
    raise SystemExit(main())
