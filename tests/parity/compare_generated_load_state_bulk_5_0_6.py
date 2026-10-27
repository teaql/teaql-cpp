#!/usr/bin/env python3
"""Compare complete generated bulk outputs, retaining every row and state result."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from compare_values import json_lines, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = ROOT / 'tests/parity/generated_load_state_bulk_5_0_6_cases.json.gz'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Use a fresh evidence path')
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    if not (corpus['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']):
        raise AssertionError()
    for relative, expected in corpus['source_hashes'].items():
        if not (digest(ROOT / relative) == expected):
            raise AssertionError(relative)
    expected_inputs = [dict(model=model,count=count,independent=independent)
                       for model in ['record','child'] for count in [1,100,10000]
                       for independent in [False,True]]
    if not (corpus['inputs']==expected_inputs and len(corpus['expected'])==12):
        raise AssertionError()
    if not (sum(len(out['rows']) for out in corpus['expected'])==40404):
        raise AssertionError()
    record = dict(scope='Full typed bulk/shared-state observations; whole-source verdict remains separate',
                  complete=False, steps=[], corpus_sha256=digest(CORPUS), source_hashes=corpus['source_hashes'])

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    payload = ''.join(json.dumps(item)+'\n' for item in corpus['inputs'])
    for label,binary in [('original',args.rust),('cpp',args.cpp)]:
        if binary is None:
            continue
        result = subprocess.run([str(binary.resolve())],input=payload,text=True,capture_output=True,
                                env=dict(os.environ,TEAQL_LOG_ENDPOINT='off'))
        record['steps'].append(dict(label=label,binary=str(binary.resolve()),sha256=digest(binary),
                                    exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        save()
        result.check_returncode()
        actual = json_lines(result.stdout)
        if not same(actual,corpus['expected']):
            record['actual']=actual
            save()
            raise RuntimeError(f'{label}: complete typed batch results differ')
    record.update(complete=True,batches=12,rows=40404,actual_original_replayed=args.rust is not None)
    save()
    print('Matched 12 complete generated batches and 40,404 row values')


if __name__=='__main__':
    main()
