#!/usr/bin/env python3
"""Compare real protected SQLite deletion/recovery and callback boundaries."""
from compare_values import diagnostic_path
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('protected_behavior_5_0_6_cases.json.gz')


def cases():
    for operation, behavior, policy, state, sink_fail, intent, trace in itertools.product(
            ('delete', 'hard_delete', 'recover'), ('inspect', 'deny', 'rewrite'),
            ('inspect', 'deny', 'rewrite'), ('matching', 'stale', 'missing', 'trigger'),
            (False, True), ('missing', 'blank', 'value'), (False, True)):
        yield dict(operation=operation, behavior=behavior, policy=policy, state=state,
                   sink_fail=sink_fail, intent=intent, trace=trace)


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
        if corpus['baseline'] != baseline or not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('Protected Behavior corpus baseline/matrix differs')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Protected Behavior corpus differs from actual original')
    actual = observe(args.cpp, inputs)
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        path = diagnostic_path(ROOT / 'docs/parity/protected_behavior_5_0_6_initial_gaps.json.gz')
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} protected Behavior differences')
    if args.record:
        corpus = dict(baseline=baseline, scope='Complete original runtime and generated Note '
                      'Behavior, real SQLite provider, source-private entity deletion/recovery '
                      'through append-only test access. RequestPolicy, explicit/missing/blank '
                      'mutation intent, rewritten versions, affected rows, raw event delivery, '
                      'trace and SQL diagnostics. Autocommit execution, not audited graph Save '
                      'or whole-plan Mutation Policy. No normalization.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(corpus, separators=(',', ':')).encode(), mtime=0))
    print(f'Protected Behavior: {len(inputs)}/{len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    main()
