"""Record original public graph planning, query boundaries and immutable batch structure."""
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


def node(entity='Parent', values=None, operation=0, dirty=None, comment=None, relations=None, old=None):
    return dict(entity=entity, values=values or {}, operation=operation, dirty=dirty,
                comment=comment, relations=relations or {}, old=old)


def descriptor(name, identity='id', version=True, timestamps=False, relations=None):
    return dict(name=name, id=identity, version=version, timestamps=timestamps,
                relations=relations if relations is not None else [dict(name='kids', target='Child')])


def case(graph, **kwargs):
    out = dict(node=graph, service='Parent', exists=False, fail_entity='', id_fail=False,
               descriptors=[descriptor('Parent'), descriptor('Child')])
    out.update(kwargs)
    return out


def inputs():
    cases = []
    ids = [None, v('null'), v('typed_null'), v('i64', '0'), v('u64', '0'),
           v('i64', '-1'), v('i64', '7'), v('u64', '7'), v('text', '7'),
           v('f64', '7.0'), v('decimal', '7.0'), v('bool', False)]
    for operation, identity, exists, dirty in itertools.product(range(4), ids,
                                                                [False, True], [None, [], ['name'], ['absent']]):
        values = dict(name=v('text', 'changed'), extra=v('list', [v('null')]), version=v('i64', '3'))
        if identity is not None:
            values['id'] = identity
        cases.append(case(node(values=values, operation=operation, dirty=dirty,
                               comment='audit', old={'version': v('i64', '99')}), exists=exists))
    for operation, exists, parent_comment, child_comment in itertools.product(range(4), [False, True],
                                                                             [None, '', 'root'], [None, '', 'child']):
        children = [node('Child', {'id': v('u64', '2'), 'name': v('text', 'x')}, 0, comment=child_comment),
                    node('Child', {'id': v('i64', '2')}, 0, relations={'invalid': [node('Wrong')]}),
                    node('Child', {'id': v('u64', '3')}, 2, comment='ignored reference'),
                    node('Child', {'id': v('u64', '4')}, 3, comment='ignored delete')]
        cases.append(case(node(values={'id': v('u64', '1')}, operation=operation,
                               comment=parent_comment, relations={'kids': children}), exists=exists))
    for identity, version, timestamps, exists in itertools.product(['', 'pk', 'id'],
                     [False, True], [False, True], [False, True]):
        values = dict(name=v('text', 'mapped'))
        if identity:
            values[identity] = v('u64', '7')
        cases.append(case(node(values=values, comment='mapped id'), exists=exists,
                          descriptors=[descriptor('Parent', identity, version, timestamps)]))
    markers = [None, v('null'), v('typed_null'), v('i64', '0'), v('u64', '0'),
               v('text', ''), v('bool', False), v('timestamp', '123'), v('i64', '-1')]
    for version, time, exists in itertools.product(markers, markers, [False, True]):
        values = dict(id=v('u64', '7'), name=v('text', 'timed'))
        if version is not None:
            values['version'] = version
        if time is not None:
            values['create_time'] = time
            values['update_time'] = time
        cases.append(case(node(values=values), exists=exists,
                          descriptors=[descriptor('Parent', timestamps=True)]))
    for operation, root_descriptor, wrong_child, children in itertools.product(range(4),
                         [False, True], [False, True], [False, True]):
        graph = node(operation=operation, relations={'kids': [node('Wrong' if wrong_child else 'Child')]} if children else {})
        cases.append(case(graph, descriptors=[descriptor('Parent')] if root_descriptor else []))
    for operation, error in itertools.product(range(4), ['', 'Parent', 'Child']):
        cases.append(case(node(values={'id': v('u64', '1')}, operation=operation,
                          relations={'kids': [node('Child', {'id': v('u64', '2')})]}), fail_entity=error))
    for operation, identity in itertools.product(range(4), [None, v('u64', '7')]):
        cases.append(case(node(operation=operation, values={'id': identity} if identity else {}), id_fail=True))
    cases.append(case(node('Other'), service='Parent'))
    cases.append(case(node(relations={'unknown': []})))
    cases.append(case(node(relations={'kids': [node('Child', operation=1,
                         relations={'kids': [node('Child', operation=3)]})]})))
    for values in [dict(id=v('typed_null')), dict(id=v('text', 'same')), dict(id=v('bool', False))]:
        cases.append(case(node(relations={'kids': [node('Child', values), node('Child', values)]})))
    for operation, exists in itertools.product([0, 1], [False, True]):
        cases.append(case(node(operation=operation, comment='pre-allocation parent',
                     relations={'kids': [node('Child', {'id': v('u64', '2')},
                                              comment='child lookup')]}), exists=exists))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'],
                  note='Original public EntityDataService::plan_graph. Mock implements read-only provider SPI, not planner logic. Automatically sampled timestamps normalize only when they lie within the observed call interval; fixed values retain exact Debug text.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('entity_graph_plan_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} entity graph planning observations')


if __name__ == '__main__':
    main()
