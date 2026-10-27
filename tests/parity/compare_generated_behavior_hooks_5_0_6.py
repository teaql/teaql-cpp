#!/usr/bin/env python3
"""Compare generated default/custom Behavior hooks through actual registration."""
from compare_values import diagnostic_path
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same
from expression_cli_cases import require_cases

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('generated_behavior_hooks_5_0_6_cases.json.gz')


def cases():
    for model, module, mode, operation, replacement, text in itertools.product(
            ('note', 'invoice', 'priority'), ('plain', 'checker', 'behavior', 'combined'),
            ('default', 'record', 'deny'), ('select', 'insert', 'update', 'delete', 'recover', 'relations'),
            (False, True), ('business', '  雪\0' * 64)):
        yield dict(model=model, module=module, mode=mode, operation=operation,
                   replacement=replacement, text=text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires actual original execution')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        expected = observe(args.rust, inputs)
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        require_cases(corpus, len(inputs), inputs)
        if corpus['baseline'] != baseline or not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('Behavior corpus baseline/matrix differs')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Behavior corpus differs from actual original')
    actual = observe(args.cpp, inputs)
    if len(actual) != len(inputs):
        raise RuntimeError('Native observations must cover the complete input matrix')
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        path = diagnostic_path(ROOT / 'docs/parity/generated_behavior_hooks_5_0_6_initial_gaps.json.gz')
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} generated Behavior differences')
    if args.record:
        corpus = dict(baseline=baseline, scope='Complete unchanged original Note/Invoice/Priority '
                      'domains and generated module factories. Actual registered default/composed '
                      'Behavior callbacks and replacement. Direct callback contract only; no SQL '
                      'execution, authorization, or governed-save claim. Rust composition versus '
                      'C++ generated-class inheritance is the declared language binding adaptation.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(corpus, separators=(',', ':')).encode(), mtime=0))
    print(f'Generated Behavior hooks: {len(inputs)}/{len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    main()
