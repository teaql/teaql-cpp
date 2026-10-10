"""Observe the actual retained recursive insert/upsert methods and complete IO."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same
from generate_graph_execution import case, node, descriptor, v


def relation(name='children', many=True, attach=True, target='Child', local='id', foreign='parent_id'):
    return dict(name=name, target=target, many=many, attach=attach, local=local, foreign=foreign)


def inputs():
    cases = []
    identities = [None, v('null'), v('u64', '0'), v('i64', '-7'), v('u64', '7'),
                  v('typed_null'), v('u64', '18446744073709551615'), v('text', 'identity🧊')]
    for mode, operation, identity, empty, comment in itertools.product(
            ['recursive_insert', 'recursive_upsert'], range(4), identities, [False, True], [None, '', 'root🧊']):
        values = {'version': v('i64', '1')}
        if identity is not None:
            values['id'] = identity
        if operation != 2:
            values['name'] = v('text', 'input parent')
        root = node(values=values, operation=operation, comment=comment)
        root.update(dirty=None, old={'name': v('text', 'caller original')})
        item = case(mode=mode, root=root, rows=[] if empty else case()['rows'],
                    trace=[dict(kind=0, entity='Ancestor', comment='parent'), dict(kind=1, entity='Blank', comment='')])
        cases.append(item)
    for mode, checker, dirty, identity, behavior in itertools.product(
            ['recursive_insert', 'recursive_upsert'], ['none', 'fill', 'erase', 'error'],
            [None, [], ['name']], [v('u64', '7'), v('null')], range(4)):
        root = node(values={'id': identity, 'version': v('i64', '1'), 'name': v('text', 'original')}, comment='root')
        root.update(dirty=dirty, old={'name': v('text', 'original snapshot')})
        item = case(mode=mode, root=root, checker=checker)
        if behavior == 1:
            item['read_batches'] = [[], case()['rows']]
        elif behavior == 2:
            item['fail'] = 0
        elif behavior == 3:
            item['event_fail'] = 0
        cases.append(item)
    child_values = {'id': v('u64', '8'), 'version': v('i64', '1'), 'name': v('text', 'child')}
    for mode, many, attach, child_operation, shape in itertools.product(
            ['recursive_insert', 'recursive_upsert'], [False, True], [False, True], range(4), range(6)):
        mapping = relation(many=many, attach=attach)
        children = [node('Child', child_values.copy(), child_operation, 'child reason')]
        if child_operation == 2:
            children[0]['values'].pop('name')
        if shape == 1:
            children = []
        elif shape == 2:
            children.append(copy.deepcopy(children[0]))
        elif shape == 3:
            children[0]['entity'] = 'Wrong'
        elif shape == 4:
            mapping['target'] = 'Missing'
        elif shape == 5:
            children[0]['values'].pop('id')
        root = node(comment='parent reason', relations={'children': children})
        item = case(mode=mode, root=root, descriptors=[descriptor(relations=[mapping]), descriptor('Child')])
        cases.append(item)
    for mode, shape in itertools.product(['recursive_insert', 'recursive_upsert'], range(10)):
        root = node(comment='parent', relations={
            'a_one': [node('Child', child_values.copy(), comment='one')],
            'z_many': [node('Child', child_values.copy(), comment='many')]})
        mappings = [relation('a_one', many=False, local='head_id', foreign='id'), relation('z_many')]
        item = case(mode=mode, root=root, descriptors=[descriptor(relations=mappings), descriptor('Child')])
        if shape in (1, 2):
            item['fail'] = shape
        elif shape in (3, 4):
            item['event_fail'] = shape - 2
        elif shape == 5:
            item['root']['relations']['unknown'] = []
        elif shape == 6:
            item['descriptors'][1]['id'] = ''
        elif shape == 7:
            item['rows'] = [{'name': v('text', 'row missing foreign/local identity')}]
        elif shape == 8:
            # Attachment to the child identity precedes canonical duplicate checks.
            item['descriptors'][0]['relations'][1]['foreign'] = 'id'
            second = node('Child', {'id': v('i64', '7'), 'version': v('i64', '1')}, 2)
            item['root']['relations']['z_many'].append(second)
        elif shape == 9:
            nested = node('Child', child_values.copy(), comment='nested')
            nested['relations'] = {'leaves': [node('Leaf', {'id': v('u64', '9'), 'name': v('text', 'leaf')}, comment='leaf')]}
            item['root']['relations']['a_one'] = [nested]
            item['descriptors'][1]['relations'] = [relation('leaves', target='Leaf')]
            item['descriptors'].append(descriptor('Leaf'))
        cases.append(item)
    for mode, identity in itertools.product(['recursive_insert', 'recursive_upsert'], ['', 'pk']):
        root = node(values={'pk': v('u64', '7'), 'name': v('text', 'custom identity')}, comment='custom')
        cases.append(case(mode=mode, root=root, descriptors=[descriptor(identity=identity)],
                          rows=[{'pk': v('u64', '7'), 'version': v('i64', '2'), 'name': v('text', 'authoritative')}]))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    cases = inputs()
    first = observe(args.rust, cases)
    assert same(first, observe(args.rust, cases)), 'Original recursive outputs differ'
    root = Path(__file__).resolve().parents[2]
    corpus = dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
                  note='Actual retained insert_graph_node_scoped/upsert_graph_node_scoped via access-only archive bridges. Complete raw graph, provider/query/event/Checker sequence and error output are compared. Controlled provider rows are fixtures, separate from real SQLite lifecycle integration.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, first)])
    with gzip.GzipFile(Path(__file__).with_name('recursive_graph_write_cases.json.gz'), 'wb', mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(',', ':')).encode())
    print(f'Recorded {len(cases)} recursive graph observations from two matching original runs')


if __name__ == '__main__':
    main()
