#!/usr/bin/env python3
"""Compare actual second-connection writes between bootstrap planning and persistence."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('bootstrap_conflict_5_0_6_cases.json.gz')


def cases():
    for module, target, conflicts, version, rounds, logging, repair in itertools.product(
            ('plain', 'checker', 'behavior', 'combined'), (1, 22), (0, 1, 2, 4, 5, 6),
            (1, 7), (1, 2), (False, True), (False, True)):
        yield dict(module=module, target=target, conflicts=conflicts, version=version,
                   rounds=rounds, logging=logging, repair=repair)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires the actual original executable')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    producer = json.loads((ROOT / 'tests/generator/parity/constants_numeric_rust/PROVENANCE.json').read_text())
    original = ROOT / 'tests/generator/parity/constants_numeric_rust'
    if (producer['runtime_commit'] != lock['commit'] or producer['hand_edited']
            or producer['model_sha256'] != hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest()
            or {p.relative_to(original).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in original.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'} != producer['files']):
        raise RuntimeError('Complete original bootstrap conflict fixture changed')
    cpp_domain = ROOT / 'tests/generated_constants_numeric'
    native = json.loads((cpp_domain / 'manifest.json').read_text())
    if (native['runtime_rust_baseline'] != lock['commit'] or native['model_sha256'] != producer['model_sha256']
            or native['generator_reference_commit'] != producer['generator_commit']
            or {p.relative_to(cpp_domain).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in cpp_domain.rglob('*') if p.is_file() and p.name != 'manifest.json'} != native['files']):
        raise RuntimeError('Complete native bootstrap conflict fixture changed')
    inputs = list(cases())
    if args.record:
        expected = observe(args.rust, inputs)
    else:
        with gzip.open(CORPUS, 'rt') as file:
            corpus = json.load(file)
        if corpus['baseline'] != lock['commit'] or not same(producer, corpus['original_provenance']):
            raise RuntimeError('Bootstrap conflict corpus source changed')
        if not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('Bootstrap conflict matrix changed')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(observe(args.rust, inputs), expected):
            raise RuntimeError('Bootstrap conflict corpus differs from actual original')
    actual = observe(args.cpp, inputs)
    differences = [dict(index=i,input=c,rust=a,cpp=b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    if differences:
        path = ROOT / 'docs/parity/bootstrap_conflict_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences),differences=differences),indent=2)+'\n').encode(),mtime=0))
        print(json.dumps(differences[0],indent=2))
        raise SystemExit(f'{len(differences)}/{len(inputs)} bootstrap conflict differences')
    if args.record:
        corpus = dict(baseline=lock['commit'],original_provenance=producer,
                      scope='Complete unchanged generated Numeric domain. Actual second SQLite connection commits after immutable Policy plan review and before transaction begin. Deterministic interleaving, not simultaneous threads. Exact rows/versions/ID floors/errors/plans/reviews/SQL intent and autocommit; no normalization.',
                      cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Bootstrap conflict: {len(inputs)}/{len(inputs)} exact second-connection outcomes')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(f'Bootstrap conflict observer failed ({error.returncode}): {error.stderr}') from error
