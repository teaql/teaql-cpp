#!/usr/bin/env python3
"""Compare generated keyword-relationship graph saves, governance and timeless safe logs."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same
from compare_relation_identity_5_0_6 import identity as relationship_identity

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('relation_graph_save_5_0_6_cases.json.gz')


def inputs():
    rows = []
    def add(target, minimal=False, selection='both', op='mixed', composition='include',
            fault='none', policy='none', audit_fail=False):
        row = dict(target=target, minimal=minimal, selection=selection, op=op,
                   composition=composition, fault=fault, policy=policy, audit_fail=audit_fail)
        if row not in rows:
            rows.append(row)
    for target, minimal, selection, op, composition in itertools.product(
            ['owner', 'entry'], [False, True], ['none', 'first', 'second', 'both', 'minimal_child'],
            ['read', 'root', 'related', 'delete', 'foreign', 'create', 'mixed'],
            ['none', 'include', 'attach']):
        add(target, minimal, selection, op, composition)
    for target, op, policy, audit_fail in itertools.product(
            ['owner', 'entry'], ['root', 'related', 'delete', 'foreign', 'create', 'mixed'],
            ['none', 'allow', 'deny'], [False, True]):
        add(target, op=op, policy=policy, audit_fail=audit_fail)
    for target, op, composition, fault in itertools.product(
            ['owner', 'entry'], ['root', 'related', 'create', 'mixed'], ['include', 'attach'],
            ['root_conflict', 'child_conflict', 'root_trigger', 'second_trigger', 'insert_trigger']):
        add(target, op=op, composition=composition, fault=fault)
    for target, op, fault, policy, audit_fail in itertools.product(
            ['owner', 'entry'], ['create', 'mixed'], ['root_conflict', 'second_trigger', 'insert_trigger'],
            ['allow', 'deny'], [False, True]):
        add(target, op=op, fault=fault, policy=policy, audit_fail=audit_fail)
    return rows


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity():
    result = relationship_identity()
    source = ROOT / 'tests/parity/rust_relation_identity_5_0_6/src/bin/relation_graph_save.rs'
    result['sources'][str(source.relative_to(ROOT))] = digest(source)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    assert args.cpp or args.record_source
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    os.environ.pop('TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS', None)
    current, cases = identity(), inputs()
    if args.record_source:
        assert args.rust and not CORPUS.exists(), 'Preserve the immutable original corpus'
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current, cases=[dict(input=i, expected=e)
                           for i, e in zip(cases, expected)]), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            assert corpus[key] == value, key
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases)), 'Original graph-save behavior changed'
    if not args.cpp:
        print('Recorded original graph-save observations:', len(cases))
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,
            original=expected, cpp=actual, failures=failures), ensure_ascii=False).encode(), mtime=0))
    for index in failures[:3]:
        print('Case', index, cases[index])
        print('Different fields:', [key for key in expected[index] if not same(expected[index][key], actual[index][key])])
    print(f'Keyword relationship graph saves: {len(cases)-len(failures)}/{len(cases)} exact original outcomes')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
