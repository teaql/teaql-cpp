#!/usr/bin/env python3
"""Compare six-variant LoadState behavior and retain full initialized diagnostics."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from compare_values import json_lines, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = ROOT / 'tests/parity/eval_indexed_5_0_6_cases.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--cpp-emulator', default='')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Use a fresh receipt path')
    data = json.loads(gzip.decompress(CORPUS.read_bytes()))
    if data['runtime_commit'] != json.loads((ROOT / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Indexed LoadState corpus belongs to a different runtime')
    for path, expected in data['source_hashes'].items():
        if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'Indexed LoadState source changed: {path}')
    if len(data['inputs']) != 18 or len(data['expected']) != 18:
        raise RuntimeError('Indexed LoadState corpus is incomplete')
    expected = [entry['common'] for entry in data['expected']]
    record = dict(complete=False, scope='18 complete six-variant state scenarios/180 mutations; raw initialized diagnostics retain existing source/Context representation differences', steps=[])
    payload = ''.join(json.dumps(item) + '\n' for item in data['inputs'])

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    for label, binary in [('original', args.rust), ('cpp', args.cpp)]:
        if binary is None:
            continue
        command = (args.cpp_emulator.split(';') if label == 'cpp' and args.cpp_emulator else []) + [str(binary.resolve())]
        result = subprocess.run(command, input=payload, text=True, capture_output=True,
                                env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
        record['steps'].append(dict(label=label, binary=str(binary.resolve()),
                                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        actual = json_lines(result.stdout)
        if len(actual) != len(data['inputs']):
            raise RuntimeError(f'{label}: indexed LoadState output count differs')
        if not same([entry['common'] for entry in actual], expected):
            raise RuntimeError(f'{label}: indexed LoadState results differ')
        for entry in actual:
            for key in ['initial_diagnostics', 'final_diagnostics']:
                diagnostics = entry[key]
                if len(diagnostics['actual']) != 4 or not all(diagnostics['actual']):
                    raise RuntimeError(f'{label}: indexed LoadState diagnostics are incomplete')
                if diagnostics['complete'] is not True:
                    raise RuntimeError(f'{label}: indexed LoadState diagnostics did not complete')
                if diagnostics['expected_snapshot_wrapper']:
                    if not same(diagnostics['actual'], diagnostics['expected_snapshot_wrapper']):
                        raise RuntimeError(f'{label}: indexed LoadState snapshot wrapper differs')
    record.update(complete=True, scenarios=18, mutations=180,
                  actual_original_replayed=args.rust is not None,
                  corpus_sha256=hashlib.sha256(CORPUS.read_bytes()).hexdigest())
    save()
    print('Matched 18 complete LoadState scenarios/180 mutations and four-format snapshot wrapping')


if __name__ == '__main__':
    main()
