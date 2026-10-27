#!/usr/bin/env python3
"""Compare generated recursive runtime attachment and local forward views."""
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
CORPUS = Path(__file__).with_name('recursive_attachment_helpers_5_0_6_cases.json.gz')


def cases():
    for model, embedded, partial, child, target, text in itertools.product(
            ('school', 'device'), (False, True), (False, True), ('plain', 'new', 'delete'),
            ('empty', 'name', 'version', 'child'), ('parent update', '  雪\0' * 64)):
        yield dict(model=model, embedded=embedded, partial=partial, child_state=child,
                   target=target, text=text)


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
            raise RuntimeError('Attachment corpus baseline/matrix differs')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Attachment corpus differs from actual original')
    native = observe(args.cpp, inputs)
    if len(native) != len(inputs):
        raise RuntimeError('Native observations must cover the complete input matrix')
    gaps = [dict(input=value, rust=source, cpp=actual)
            for value, source, actual in zip(inputs, expected, native) if not same(source, actual)]
    if gaps:
        path = diagnostic_path(ROOT / 'docs/parity/recursive_attachment_helpers_5_0_6_initial_gaps.json.gz')
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} recursive attachment differences')
    if args.record:
        corpus = dict(baseline=baseline, scope='Complete unchanged original relations domain. '
                      'Actual required/optional forward objects, generated recursive attachment, '
                      'source/target ledger values, comments/new/delete/original versions, local '
                      'snapshot/eval/load state and post-attachment shared updates. Conflicting '
                      'target version observes actual public state; no save/SQL claim or normalization.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(corpus, separators=(',', ':')).encode(), mtime=0))
    print(f'Recursive attachment: {len(inputs)}/{len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    main()
