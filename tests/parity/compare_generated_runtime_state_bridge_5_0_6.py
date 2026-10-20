#!/usr/bin/env python3
"""Compare actual generated borrowed-root identity and shared ledger observations."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from compare_values import same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = ROOT / 'tests/parity/generated_runtime_state_bridge_5_0_6_cases.json.gz'


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
    assert corpus['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    for path, expected in corpus['source_hashes'].items():
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == expected, path
    assert len(corpus['inputs']) == len(corpus['expected']) == 6
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
        assert result.stdout.endswith('\n')
        actual = [json.loads(line) for line in result.stdout.split('\n')[:-1]]
        assert same(actual, corpus['expected']), label
    record.update(complete=True, observations=6, actual_original_replayed=args.rust is not None,
                  corpus_sha256=hashlib.sha256(CORPUS.read_bytes()).hexdigest())
    save()
    print('Matched6 actual generated model borrowed-root and shared ledger observations')


if __name__ == '__main__':
    main()
