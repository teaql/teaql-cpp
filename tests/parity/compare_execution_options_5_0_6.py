#!/usr/bin/env python3
"""Compare generated optimization constructors, defaults and validation order."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from compare_composed_general_5_0_6 import identity as source_identity
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('execution_options_5_0_6_cases.json.gz')
MAX = str(2**64 - 1)
BLANK = ['', ' \t\r\n', '\u0085', '\u00a0', '\u1680', '\u2000', '\u2028',
         '\u2029', '\u202f', '\u205f', '\u3000']
NONBLANK = ['\u200b', '\ufeff', '  scoped namespace\t', 'name\0suffix',
            'long namespace ' * 32]


def continuous(namespace='default', ttl='600'):
    return dict(op='continuous_config', namespace=namespace, ttl=ttl)


def id_set(namespace='default', ttl='600', maximum='3000000'):
    return dict(op='id_set_config', namespace=namespace, ttl=ttl, max=maximum)


def inputs():
    cases = []
    for entity in ['school', 'academy', 'research_academy', 'enrollment']:
        seed = [continuous('retained continuous', '7'),
                id_set('retained ids', '9', '11'), dict(op='top_n', threshold='7')]
        cases.append(dict(entity=entity, actions=[
            dict(op='continuous_default'), continuous(),
            dict(op='id_set_default'), id_set(),
            dict(op='top_n', threshold='0'), dict(op='top_n', threshold='1'),
            dict(op='top_n', threshold=MAX), continuous('large', MAX),
            id_set('large', MAX, MAX), dict(op='continuous_default'),
            dict(op='id_set_default')]))
        for namespace in BLANK + NONBLANK:
            for action in [continuous(namespace, '1'), id_set(namespace, '1', '1')]:
                cases.append(dict(entity=entity, actions=seed + [action]))
        for action in [continuous('', '0'), continuous('valid', '0'),
                       id_set('', '0', '0'), id_set('valid', '0', '0'),
                       id_set('valid', '1', '0')]:
            cases.append(dict(entity=entity, actions=seed + [action]))
    return cases


def identity():
    value = source_identity()
    for name in ['tests/parity/rust_dynamic_json_privacy_5_0_6/src/bin/execution_options.rs',
                 '.reference/teaql-rs/teaql-core/src/query.rs']:
        value['sources'][name] = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
    return value


def check_source(cases, results):
    errors = 0
    steps = 0
    for case, result in zip(cases, results):
        assert len(case['actions']) == len(result)
        for action, step in zip(case['actions'], result):
            expected = None
            if action['op'] in {'continuous_config', 'id_set_config'}:
                prefix = ('continuous page' if action['op'] == 'continuous_config'
                          else 'ID set pagination')
                if action['namespace'] in BLANK:
                    expected = prefix + ' namespace must not be empty'
                elif action['ttl'] == '0':
                    expected = prefix + ' ttl_seconds must be positive'
                elif action.get('max') == '0':
                    expected = prefix + ' max_ids must be positive'
            assert step['error'] == expected, (case, action, step)
            if expected:
                assert step['before'] == step['after'], (case, action)
                errors += 1
            steps += 1
        if len(case['actions']) == 11:
            assert result[0]['after'] == result[1]['after']
            assert result[2]['after'] == result[3]['after']
    return dict(cases=len(cases), steps=steps, rejected=errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    current, cases = identity(), inputs()
    if args.record_source:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        check_source(cases, expected)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current, cases=[
            dict(input=i, expected=e) for i, e in zip(cases, expected)]),
            ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            assert same(corpus[key], value), key
        assert same(cases, [row['input'] for row in corpus['cases']])
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases))
    counts = check_source(cases, expected)
    if not args.cpp:
        print('Actual original optimization options:', counts)
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(
            dict(inputs=cases, original=expected, cpp=actual, failures=failures),
            ensure_ascii=False).encode(), mtime=0))
    for i in failures[:3]:
        print('Case', i, cases[i], '\nRust:', expected[i], '\nC++:', actual[i])
    if not failures:
        check_source(cases, actual)
    print(f'Optimization options: {len(cases)-len(failures)}/{len(cases)} exact; {counts}')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
