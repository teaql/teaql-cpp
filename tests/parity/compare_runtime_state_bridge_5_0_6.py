#!/usr/bin/env python3
"""Compare actual automatic-save dispatch and complete language-specific diagnostics."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from compare_values import same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = ROOT / 'tests/parity/runtime_state_bridge_5_0_6_cases.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--cpp-emulator', default='')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Use a fresh receipt path')
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    if corpus['runtime_commit'] != json.loads((ROOT / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('State bridge corpus belongs to a different runtime')
    for path, expected in corpus['source_hashes'].items():
        if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'State bridge source changed: {path}')
    if (len(corpus['inputs']) != 24 or len(corpus['source_expected']) != 24
            or len(corpus['cpp_expected']) != 24):
        raise RuntimeError('State bridge corpus is incomplete')
    record = dict(complete=False, scope=corpus['scope'], steps=[])
    payload = ''.join(json.dumps(item) + '\n' for item in corpus['inputs'])

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    for label, binary in [('source', args.rust), ('cpp', args.cpp)]:
        if binary is None:
            continue
        command = (args.cpp_emulator.split(';') if label == 'cpp' and args.cpp_emulator else []) + [str(binary.resolve())]
        result = subprocess.run(command, input=payload, text=True, capture_output=True,
                                env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
        record['steps'].append(dict(label=label, command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr,
                                    sha256=hashlib.sha256(binary.read_bytes()).hexdigest()))
        save()
        result.check_returncode()
        if not result.stdout.endswith('\n'):
            raise RuntimeError(f'{label}: state bridge output is not LF-terminated')
        actual = [json.loads(line) for line in result.stdout.split('\n')[:-1]]
        if not same(actual, corpus[label + '_expected']):
            raise RuntimeError(f'{label}: complete state bridge results differ')
    record.update(complete=True, observations=24, actual_original_replayed=args.rust is not None,
                  corpus_sha256=hashlib.sha256(CORPUS.read_bytes()).hexdigest())
    save()
    print('Matched24 full automatic-save dispatch, hook-count and raw error observations')


if __name__ == '__main__':
    main()
