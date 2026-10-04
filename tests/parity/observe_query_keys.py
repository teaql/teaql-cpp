#!/usr/bin/env python3
"""Record actual pinned canonicalization and pagination-key methods."""
import argparse
import copy
import itertools
import json
import random
from pathlib import Path
from compare_values import observe


def value(kind, data=None):
    return dict(kind=kind, **({} if data is None else dict(value=data)))


def literal(v):
    return dict(kind='value', value=v)


def column(name):
    return dict(kind='column', name=name)


def binary(op, left, right):
    return dict(kind='binary', op=op, left=left, right=right)


def cases():
    result = []

    def add(query, **identity):
        record = dict(query=query, namespace='tenant', query_namespace='entity:Device', user='user')
        record.update(identity)
        result.append(copy.deepcopy(record))

    scalar = [value('null'), value('typed_null'), value('bool', True),
              value('i64', '-9223372036854775808'), value('u64', '18446744073709551615'),
              value('f64', '-0'), value('f64', '0'), value('f64', 'NaN'),
              value('f64', 'inf'), value('f64', '-inf'), value('decimal', '1.00'),
              value('decimal', '1.0'), value('text', 'long Unicode \u03a3\u96ea\0\u2028'),
              value('json', dict(secret='credential', number=1.0, nested=[None, False])),
              value('date', '2024-02-29'), value('timestamp', '-9223372036854775808'),
              value('object', dict(z=value('text', 'nested'), a=value('u64', '1'))),
              value('list', [value('i64', '1'), value('u64', '1')])]
    for v in scalar:
        add(dict(filter=literal(v)))
        for op in range(8, 12):
            add(dict(filter=binary(op, column('id'), literal(value('list', [v, v] + scalar)))))
    for op in range(12):
        add(dict(filter=binary(op, column('field'), literal(value('text', 'secret')))))
    for f in range(14):
        add(dict(expr_projection=[dict(alias='expr', expr=dict(kind='function', function=f,
                    args=[literal(scalar[f % len(scalar)]), column('field')]))]))
    for f in range(12):
        add(dict(aggregates=[dict(function=f, field='field', alias='stats')]))
    for kind in ['null', 'not_null', 'not']:
        add(dict(filter=dict(kind=kind, args=[column('field')])) )
    add(dict(filter=dict(kind='between', args=[column('f'), literal(scalar[3]), literal(scalar[4])])) )
    predicates = [binary(0, column('a'), literal(value('u64', '1'))),
                  binary(2, column('version'), literal(value('i64', '0'))),
                  binary(8, column('id'), literal(value('list', [value('u64', '2'), value('u64', '1'), value('u64', '2')])))]
    for order in itertools.permutations(predicates):
        for kind in ['and', 'or']:
            add(dict(filter=dict(kind=kind, args=list(order) + [dict(kind=kind, args=[order[0]])])))
    for kind in ['and', 'or']:
        add(dict(filter=dict(kind=kind, args=[])))
        add(dict(filter=dict(kind=kind, args=[predicates[0]])))

    descriptor = dict(name='Descriptor', table='quoted table', service='local service',
                      declared=True, masked=['secret', 'secret'], audit_max=123,
                      properties=[dict(name=f'field{i}', type=i, column=f'column{i}', nullable=i % 2 == 0,
                                       id=i == 2, version=i == 1, length=i, precision=32, scale=0)
                                  for i in range(10)],
                      relations=[dict(name='children', target='Child', local='id', foreign='parent_id',
                                      many=True, attach=False, delete_missing=False)])
    child = dict(entity='Child', filter=dict(kind='and', args=list(reversed(predicates))),
                 comment='child comment', cache=dict(enabled=True, age=8, propagate=True, propagate_age=13),
                 stream=0, continuous=dict(namespace='child-page', ttl=12),
                 ids=dict(namespace='child-ids', ttl=13, max=14),
                 trace=[dict(kind=2, entity='Child', comment='relation', id=12)])
    full = dict(entity='Device', hard_limit=18446744073709551615, projection=['z', 'a', 'z'],
                expr_projection=[dict(alias='expr', expr=dict(kind='subquery', op=8, left=column('id'),
                     entity=descriptor, query=child))], search='search Unicode \u03a3', filter=dict(kind='and', args=predicates),
                having=dict(kind='or', args=list(reversed(predicates))),
                order=[dict(direction=1, field='id'), dict(direction=0, expr=dict(kind='function', function=1, args=[column('name')]))],
                slice=dict(offset=18446744073709551615, limit=0), partition='parent', threshold=0,
                aggregates=[dict(function=0, field='*', alias='count')], group=['g', 'g'],
                relations=[dict(name='children', query=child), dict(name='empty')],
                cache=dict(enabled=False, age=18446744073709551615, propagate=True, propagate_age=0),
                comment='comment', trace=[dict(kind=i, entity='Device', comment='trace', **(dict(id=i) if i % 2 else {})) for i in range(9)],
                raw_sql='SELECT secret FROM devices', raw_criteria=['x', 'y'],
                raw_projection=[dict(alias='raw', sql='expression')], dynamic=[dict(alias='dynamic', sql='dynamic expression')],
                object_groups=[dict(property='obj', storage='obj_id', query=child)], children=[child],
                stream=18446744073709551615, continuous=dict(namespace='outer-page', ttl=22),
                ids=dict(namespace='outer-ids', ttl=33, max=44))
    add(full)
    for attr in list(full):
        q = copy.deepcopy(full)
        del q[attr]
        add(q)
    for target in ['filter', 'having']:
        for op in [8, 9, 10, 11]:
            add(dict(**{target: dict(kind='subquery', op=op, left=column('id'), entity=descriptor, query=full)}))
    for size in range(32):
        for user in [None, '', 'user', 'Unicode \u96ea\0identity']:
            add(dict(entity='e' * size), namespace='n' * size, user=user,
                root=dict(entity='r' * size, id=18446744073709551615))
    for root in [None, dict(entity='', id=0), dict(entity='Root', id=0), dict(entity='Root', id=1)]:
        for user in [None, '', 'user']:
            identity = dict(user=user)
            if root is not None:
                identity['root'] = root
            add(full, **identity)

    rng = random.Random(0x505CA11)
    for i in range(128):
        selected = [copy.deepcopy(rng.choice(predicates)) for _ in range(rng.randrange(0, 8))]
        nested = dict(kind=rng.choice(['and', 'or']), args=selected)
        q = dict(entity=f'Entity{i % 5}', filter=dict(kind=rng.choice(['and', 'or']), args=[nested] + selected),
                 having=literal(rng.choice(scalar)), slice=dict(offset=rng.randrange(10000), limit=rng.randrange(100)),
                 comment=str(i), threshold=i % 3, stream=i % 7,
                 order=[dict(direction=i % 2, expr=copy.deepcopy(nested))])
        if i % 3 == 0:
            q['relations'] = [dict(name='children', query=copy.deepcopy(child))]
        add(q, namespace=rng.choice(['', 'tenant', 'tenant::x', 'unicode\0\u96ea']), user=rng.choice([None, '', 'user']))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    inputs = cases()
    outputs = observe(args.rust, inputs)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  cases=[dict(input=i, expected=o) for i, o in zip(inputs, outputs)])
    Path(__file__).with_name('query_keys_cases.json').write_text(json.dumps(corpus, ensure_ascii=True, separators=(',', ':')) + '\n')
    print(f'Recorded {len(inputs)} canonical and pagination query-key observations')


if __name__ == '__main__':
    main()
