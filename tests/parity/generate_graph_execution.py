"""Record graph-plan execution, governed saves and authoritative root reads."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def v(kind, value=None):
    out = dict(kind=kind)
    if value is not None:
        out['value'] = value
    return out


def node(entity='Order', values=None, operation=0, comment=None, relations=None):
    return dict(entity=entity, values=values if values is not None else {'id': v('u64', '7'), 'name': v('text', 'planned')},
                operation=operation, comment=comment, relations=relations or {})


def item(kind=0, values=None, fields=None, entity='Order', token=None, old=None):
    return dict(entity=entity, kind=kind,
                values=values if values is not None else {'id': v('u64', '7'), 'version': v('i64', '1'), 'name': v('text', 'new')},
                fields=['name'] if fields is None else fields, token=token or [], old=old)


def descriptor(entity='Order', identity='id', relations=None):
    return dict(entity=entity, id=identity, relations=relations or [])


def case(**kwargs):
    out = dict(mode='execute', service='Order', root=node(), items=[item()],
               descriptors=[descriptor(), descriptor('Child')],
               rows=[{'id': v('u64', '7'), 'version': v('i64', '9'), 'name': v('text', 'database')}],
               counts=[1, 1], fail=-1, event_fail=-1, read_fail=False,
               entity='Order', property='id', id=v('u64', '7'), trace=[])
    out.update(kwargs)
    return out


def inputs():
    cases = [case(root=None)]
    values = [{}, {'id': v('u64', '7')}, {'id': v('u64', '7'), 'version': v('null')},
              {'id': v('u64', '7'), 'version': v('i64', '1'), 'name': v('text', 'new')},
              {'id': v('u64', '7'), 'version': v('u64', '1'), 'name': v('text', 'new')},
              {'id': v('u64', '7'), 'version': v('text', '1'), 'name': v('text', 'new')},
              {'id': v('typed_null'), 'version': v('i64', '1')},
              {'id': v('i64', '-7'), 'name': v('text', 'negative')}]
    for kind, value, fields, count, removed in itertools.product(range(4), values,
                                                                [[], ['name']], [0, 1, 2], [False, True]):
        cases.append(case(items=[item(kind, value, fields)], counts=[count],
                          root=node(operation=3 if removed else 0)))
    for kind, error, event_error in itertools.product(range(3), [-1, 0, 1], [-1, 0, 1]):
        cases.append(case(items=[item(kind), item(kind, {'id': v('u64', '8'), 'version': v('i64', '2'), 'name': v('text', 'second')})],
                          fail=error, event_fail=event_error))
    for kind in [1, 2]:
        cases.append(case(items=[item(kind), item(kind, {'name': v('text', 'missing second ID')})]))
    for root, descriptors, rows, error in itertools.product(
            [node(values={}), node(), node(values={'pk': v('u64', '7')})],
            [[], [descriptor(identity='')], [descriptor()], [descriptor(identity='pk')]],
            [[], [{'name': v('text', 'partial database row')}]], [False, True]):
        cases.append(case(items=[], root=root, descriptors=descriptors, rows=rows, read_fail=error))
    token = [dict(entity='Parent', comment='ancestor', index=0), dict(entity='Order', comment='', index=1)]
    for kind, comment in itertools.product(range(4), [None, '', 'reason']):
        cases.append(case(items=[item(kind, token=token, old={'name': v('text', 'old')})], root=node(comment=comment,
                          relations={'unchanged': [node('Child', {'id': v('u64', '8'), 'name': v('text', 'unread child')})]})))
    for count, error, rows in itertools.product([0, 1, 2], [False, True], [[],
            [{'id': v('u64', '1')}, {'id': v('u64', '2')}]]):
        cases.append(case(mode='fetch', items=[], counts=[count], read_fail=error, rows=rows,
                          trace=[dict(kind=0, entity='Order', comment='reread')]))
    for operation, rows, error, event_error in itertools.product(range(4), [[],
            [{'id': v('u64', '7'), 'version': v('i64', '1'), 'name': v('text', 'db')}]],
            [-1, 0], [-1, 0]):
        cases.append(case(mode='save', root=node(operation=operation, comment='save'),
                          rows=rows, fail=error, event_fail=event_error))
    cases.append(case(mode='save', root=node('Other')))
    cases.append(case(items=[item(0), item(3), item(1), item(2)]))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'],
                  note='Original execute_graph_plan_internal, save_graph_internal and fetch_graph_current_row_internal via access-only archive bridges. Fixtures construct models and implement provider/event SPIs; source execution, governance and reread bodies remain unchanged.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('graph_execution_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} graph-execution observations')


if __name__ == '__main__':
    main()
