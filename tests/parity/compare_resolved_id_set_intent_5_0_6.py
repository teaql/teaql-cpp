#!/usr/bin/env python3
"""Compare ID-set intent execution; mismatches are evidence, never passing parity."""
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
    original_path = ROOT / 'docs/parity/resolved_id_set_intent_original_v2_5_0_6.json.gz'
    original = json.loads(gzip.decompress(original_path.read_bytes()))
    observer = ROOT / 'tests/parity/resolved_id_set_intent_source_5_0_6.rs'
    source = ROOT / '.reference/teaql-rs/teaql-runtime/src/data_service/resolved.rs'
    if (not original['complete']
            or not original['provenance']['all_original_bodies_preserved']
            or original['observer_sha256'] != digest(observer)
            or original['provenance']['source_files_sha256']['src/data_service/resolved.rs'] != digest(source)):
        raise RuntimeError('Original observation provenance does not match current inputs')
    rows = original['observations']
    historical_path = ROOT / 'tests/parity/id_set_page_5_0_6_cases.json.gz'
    historical = json.loads(gzip.decompress(historical_path.read_bytes()))
    if original['historical_corpus_sha256'] != digest(historical_path):
        raise RuntimeError('Historical corpus changed')
    if len(rows) != 466 or [row['case'] for row in rows] != list(range(466)):
        raise RuntimeError('Incomplete source observations')
    old_inputs = [case['input'] for case in historical['cases']]
    inputs = old_inputs + [dict(value, purpose='Verify retained ID-set execution') for value in old_inputs]
    if not same([row['input'] for row in rows], inputs):
        raise RuntimeError('Input series is not the exact historical plus explicit-purpose series')
    if not same([row['expected'] for row in rows[:233]], [case['expected'] for case in historical['cases']]):
        raise RuntimeError('Historical original outcomes changed')
    expected = [row['expected'] for row in rows]
    positive = [row for row in expected[233:] if isinstance(row, dict)]
    physical_calls = sum(len(row['requests']) for row in positive)
    writes = sum(sum(note.startswith('put:') for note in row['store']) for row in positive)
    provider_errors = sum(result.get('error') == 'ID lookup failed'
                          for row in positive for result in row['results'])
    if (physical_calls, writes, provider_errors) != (153, 61, 8):
        raise RuntimeError('Source continuation no longer reaches the qualified provider/store paths')
    executable = args.cpp.resolve()
    before = digest(executable)
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    result = subprocess.run([*emulator, str(executable)], capture_output=True, text=True, check=True,
                            input="".join(json.dumps(value)+"\n" for value in inputs))
    actual = [json.loads(line) for line in result.stdout.splitlines()]
    if len(expected) != 466 or len(actual) != len(expected):
        raise RuntimeError('Incomplete observation set')
    if digest(executable) != before:
        raise RuntimeError('Executable changed during observation')
    differences = [{'case': index, 'input': inputs[index], 'rust': left, 'cpp': right}
                   for index, (left, right) in enumerate(zip(expected, actual)) if not same(left, right)]
    paths = ['tests/parity/id_set_page_probe.cpp',
             'teaql_runtime/include/teaql/runtime/detail/data_service/entity_query.hpp',
             'teaql_runtime/include/teaql/runtime/detail/data_service/id_set_page.hpp',
             'teaql_runtime/src/data_service/query_keys.cpp',
             'tests/parity/compare_values.py', 'tests/CMakeLists.txt']
    record = dict(complete=True, parity_passed=not differences,
                  baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                  original_receipt_sha256=digest(original_path),
                  controller=Path(__file__).read_text(),
                  source_sha256={path: digest(ROOT/path) for path in paths},
                  executable_sha256=before, observations=actual, inputs=inputs,
                  differences=differences, stderr=result.stderr,
                  matched=len(expected)-len(differences), total=len(expected),
                  positive_provider_calls=physical_calls, positive_store_puts=writes,
                  positive_provider_errors=provider_errors)
    if args.output:
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))
    print(f"ID-set intent execution: {record['matched']}/{record['total']} match")
    return bool(differences)


if __name__ == '__main__':
    raise SystemExit(main())
