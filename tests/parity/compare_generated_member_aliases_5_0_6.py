#!/usr/bin/env python3
"""Compare real generated ordinary/inherited decoders against locked source results."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
CORPUS = ROOT / 'tests/parity/generated_member_alias_5_0_6_cases.json.gz'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path, help='Also replay the actual original generated Rust models')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Use a fresh evidence path')
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    if corpus['runtime_commit'] != lock['commit']:
        raise RuntimeError('Corpus runtime baseline differs')
    for relative, expected in corpus['source_hashes'].items():
        if digest(ROOT / relative) != expected:
            raise RuntimeError(f'Original source/observer changed: {relative}')
    if len(corpus['inputs']) != 260 or len(corpus['expected']) != 260:
        raise RuntimeError('Incomplete generated source corpus')
    record = dict(scope='Actual generated decoder alias subset; not whole G15', complete=False,
                  corpus_sha256=digest(CORPUS), steps=[], source_hashes=corpus['source_hashes'])

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    payload = ''.join(json.dumps(row) + '\n' for row in corpus['inputs'])
    for label, binary in [('original', args.rust), ('cpp', args.cpp)]:
        if binary is None:
            continue
        process = subprocess.run([str(binary.resolve())], input=payload, text=True, capture_output=True,
                                 env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
        record['steps'].append(dict(label=label, binary=str(binary.resolve()), sha256=digest(binary),
                                    exit_code=process.returncode, stdout=process.stdout, stderr=process.stderr))
        save()
        process.check_returncode()
        actual = [json.loads(line) for line in process.stdout.splitlines()]
        if len(actual) != len(corpus['expected']) or actual != corpus['expected']:
            record['actual'] = actual
            save()
            raise RuntimeError(f'{label}: complete generated decoder observations differ')
    record['observations'] = 260
    record['actual_original_replayed'] = args.rust is not None
    record['complete'] = True
    save()
    print('Matched260 complete generated ordinary/inherited alias observations')


if __name__ == '__main__':
    main()
