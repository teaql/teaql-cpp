#!/usr/bin/env python3
"""Replay explicit 5.0.6 mutation requests through unchanged original SQL execution."""
import argparse
from copy import deepcopy
import hashlib
import gzip
import itertools
import json
from pathlib import Path
import subprocess

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    result = []
    def entry(request, path='root'):
        request['comment'] = f'Explicit {path} {request["mode"]} entry'
        for index, child in enumerate(request.get('children', [])):
            entry(child, f'{path}.{index}')
    for corpus in ['sql_execution_cases.json', 'sql_transaction_cases.json', 'sql_executor_kind_cases.json']:
        for case in json.loads((Path(__file__).parent / corpus).read_text())['cases']:
            source = deepcopy(case['input'])
            entry(source['request'])
            result.append(source)
    entity = deepcopy(result[0]['entity'])
    text = lambda value: dict(kind='text', value=value)
    u64 = lambda value: dict(kind='u64', value=str(value))
    first = dict(mode='update', id=u64(7), values={'name': text('CURRENT-NAME')},
                 trace_nodes=[dict(kind=2, entity='items', id=None, comment='ancestor.items'),
                              dict(kind=8, entity='Ancestor', id=42, comment='stale root'),
                              dict(kind=8, entity='Account', id=7, comment='local item')])
    future = dict(mode='update', id=u64(8), values={'tenant': u64(2)},
                  old_values={'name': text('OLD-NAME-CANARY'), 'token': text('OLD-CREDENTIAL-CANARY')},
                  comment='future child')
    for middle, leaf, transaction, observe_flag, mode in itertools.product(
            ['request root', 'middle reason'], ['request root', 'middle reason', 'leaf reason'],
            [False, True], [False, True], ['success', 'error', 'pending', 'panic', 'read_error']):
        child = deepcopy(first); child['comment'] = leaf
        nested = dict(mode='batch', comment=middle, children=[child])
        root = dict(mode='batch', comment='request root', children=[nested, deepcopy(future)])
        steps = [dict(mode=mode)] if mode != 'read_error' else [dict(mode='success'), dict(mode='error')]
        result.append(dict(entity=entity, request=root, transaction=transaction, observe=observe_flag, steps=steps))
    for missing in [False, True]:
        child = deepcopy(first); child['comment'] = 'first child'
        sibling = deepcopy(future)
        if missing: sibling['entity'] = 'Unknown'
        root = dict(mode='batch', comment='root OLD-NAME-CANARY OLD-CREDENTIAL-CANARY', children=[child, sibling])
        for transaction, mode in itertools.product([False, True], ['success', 'error', 'read_error']):
            steps = [dict(mode=mode)] if mode != 'read_error' else [dict(mode='success'), dict(mode='error')]
            result.append(dict(entity=entity, request=root, transaction=transaction, steps=steps))
    for transaction, observe_flag, mode in itertools.product([False, True], [False, True], ['success', 'error', 'pending', 'read_error']):
        child = deepcopy(first); child['comment'] = 'child PRIVATE-CHILD'; child['trusted_redactions'] = [text('PRIVATE-CHILD')]
        sibling = deepcopy(future); sibling['trusted_redactions'] = [text('PRIVATE-SIBLING')]
        nested = dict(mode='batch', comment='nested PRIVATE-NESTED', children=[child], trusted_redactions=[text('PRIVATE-NESTED')])
        root = dict(mode='batch', comment='root PRIVATE-ROOT PRIVATE-SIBLING', children=[nested, sibling], trusted_redactions=[text('PRIVATE-ROOT')])
        steps = [dict(mode=mode)] if mode != 'read_error' else [dict(mode='success'), dict(mode='error')]
        result.append(dict(entity=entity, request=root, transaction=transaction, observe=observe_flag, steps=steps))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    for index, (left, right) in enumerate(zip(expected, actual)):
        if not same(left, right):
            Path('/tmp/teaql_sql_execution_506_mismatch.json').write_text(json.dumps(
                dict(index=index, input=inputs[index], rust=left, cpp=right), indent=2) + '\n')
            raise SystemExit(f'Execution mismatch {index}; inspect /tmp/teaql_sql_execution_506_mismatch.json')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    sources = ['teaql-sql/src/mutation_execution.rs', 'teaql-sql/src/executor.rs', 'teaql-sql/src/diagnostic_execution.rs',
               'teaql-data-service/src/lib.rs', 'teaql-data-service/src/sql_log.rs', 'teaql-sql/src/bindings.rs']
    corpus = dict(baseline=lock['commit'], version=lock['version'],
        source_sha256={source: hashlib.sha256((ROOT / '.reference/teaql-rs' / source).read_bytes()).hexdigest() for source in sources},
        note='Actual original public SQL executor calls with explicit entry intent. Historical inputs replayed into a new corpus; original 5.0.5 evidence remains unchanged. Complete trace nodes, physical metadata, privacy, bind/order/error/cancellation behavior. No output normalization.',
        cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('sql_execution_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} original Rust/C++ SQL execution observations')


if __name__ == '__main__':
    main()
