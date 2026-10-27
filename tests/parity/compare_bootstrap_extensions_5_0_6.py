#!/usr/bin/env python3
"""Compare generated bootstrap with application checker/behavior/resource overrides."""
from compare_values import diagnostic_path
from expression_cli_cases import require_cases
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
CORPUS = Path(__file__).with_name('bootstrap_extensions_5_0_6_cases.json.gz')


def cases():
    for module, state, checker, behavior, delta, rounds, continuation in itertools.product(
            ('plain', 'checker', 'behavior', 'combined'),
            ('fresh', 'existing', 'drift', 'missing', 'deleted', 'extra'),
            ('none', 'observe', 'fix', 'deny'), ('none', 'observe', 'deny'), (1, 9), (1, 2), (False, True)):
        tag = 'resource-' + ('r' * 128 if delta == 1 else '\u96ea' * 64)
        yield dict(module=module, state=state, checker=checker, behavior=behavior,
                   delta=delta, tag=tag, rounds=rounds, continuation=continuation)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires actual unchanged original execution')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    producer = json.loads((ROOT / 'tests/generator/parity/constants_numeric_rust/PROVENANCE.json').read_text())
    original = ROOT / 'tests/generator/parity/constants_numeric_rust'
    if (producer['runtime_commit'] != baseline or producer['hand_edited']
            or producer['model_sha256'] != hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest()
            or {p.relative_to(original).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in original.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'} != producer['files']):
        raise RuntimeError('Complete original bootstrap extension fixture changed')
    cpp_domain = ROOT / 'tests/generated_constants_numeric'
    native = json.loads((cpp_domain / 'manifest.json').read_text())
    if (native['runtime_rust_baseline'] != baseline or native['model_sha256'] != producer['model_sha256']
            or native['generator_reference_commit'] != producer['generator_commit']
            or {p.relative_to(cpp_domain).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in cpp_domain.rglob('*') if p.is_file() and p.name != 'manifest.json'} != native['files']):
        raise RuntimeError('Complete native bootstrap extension fixture changed')
    inputs = list(cases())
    if args.record:
        expected = observe(args.rust, inputs)
    else:
        with gzip.open(CORPUS, 'rt') as file:
            corpus = json.load(file)
        require_cases(corpus, len(inputs), inputs)
        if corpus['baseline'] != baseline or not same(corpus['original_provenance'], producer):
            raise RuntimeError('Bootstrap extension corpus source differs')
        if not same(inputs, [case['input'] for case in corpus['cases']]):
            raise RuntimeError('Bootstrap extension matrix differs')
        expected = [case['expected'] for case in corpus['cases']]
        if args.rust and not same(observe(args.rust, inputs), expected):
            raise RuntimeError('Bootstrap extension corpus differs from live original')
    actual = observe(args.cpp, inputs)
    gaps = [dict(index=i,input=c,rust=a,cpp=b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    if gaps:
        path = diagnostic_path(ROOT / 'docs/parity/bootstrap_extensions_5_0_6_initial_gaps.json.gz')
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+'\n').encode(),mtime=0))
        print(json.dumps(gaps[0],indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} bootstrap extension differences')
    if args.record:
        corpus=dict(baseline=baseline,original_provenance=producer,
                    scope='Complete unchanged original Numeric domain with registered custom generated CheckerLogic/TypedChecker, before_select behavior through generated Q, before_insert/update through real public command preparation, and context-owned typed resource. Ordinary audited graph save follows its separate source hook contract. Exact hooks/status/scalars/prepared commands/rows/errors/SQL intent and actual resource Drop counts; no normalization. Prepared commands are not persisted.',
                    cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Bootstrap extensions: {len(inputs)}/{len(inputs)} exact override/resource outcomes')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(f'Extension observer failed ({error.returncode}): {error.stderr}') from error
