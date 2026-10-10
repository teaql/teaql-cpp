"""Record actual retained adapter bodies through access-only archive bridges."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same
from generate_graph_execution import case, node, descriptor, v


def inputs():
    modes = ['adapter_graph', 'adapter_status', 'adapter_status_comment',
             'adapter_comment', 'adapter_create']
    out = []
    for mode, status, deleted, shape in itertools.product(modes, range(7), [False, True], range(4)):
        item = case(mode=mode, status=status, deleted=deleted, annotation=['', 'override🧊'][shape % 2],
                    root=node(comment='embedded model comment'), dirty=['name'] if shape & 1 else None,
                    old={'name': v('text', 'original allocated payload')} if shape & 2 else None)
        if shape & 2:
            item['root']['values'].update({'_comment': v('text', 'embedded value comment'),
                                           '_dirty_fields': v('list', [v('text', 'embedded')]),
                                           '_original_values': v('object', {'name': v('text', 'embedded old')})})
        out.append(item)
    for mode, status, error in itertools.product(modes, range(7), range(6)):
        item = case(mode=mode, status=status, deleted=False, annotation='error boundary',
                    dirty=None, old=None)
        if error == 0:
            item['service'] = 'Other'
        elif error == 1:
            item['descriptors'] = []
        elif error == 2:
            item['rows'] = []
        elif error == 3:
            item['read_fail'] = True
        elif error == 4:
            item['fail'] = 0
        else:
            item['event_fail'] = 0
        out.append(item)
    for mode, status in itertools.product(modes, range(7)):
        item = case(mode=mode, status=status, deleted=True, annotation='invalid child',
                    dirty=[], old={}, descriptors=[descriptor(relations=[dict(name='kids', target='Child')]), descriptor('Child')])
        item['root']['values']['kids'] = v('bool', True)
        out.append(item)
    def core(entity='Order', operation=0, children=None):
        return dict(entity=entity, operation=operation, comment='core annotation',
                    values={'id': v('u64', '7'), 'name': v('text', 'core input'),
                            '_comment': v('text', 'ordinary core value')}, children=children or [])
    for operation, shape in itertools.product(range(2), range(4)):
        children = []
        if shape:
            children = [dict(name='kids', node=core('Child', shape % 2)),
                        dict(name='kids', node=core('Child', (shape + 1) % 2))]
        if shape == 2:
            children.append(dict(name='a', node=core('Child')))
        item = case(mode='adapter_core', core=core(operation=operation, children=children),
                    descriptors=[descriptor(relations=[dict(name='kids', target='Child'), dict(name='a', target='Child')]), descriptor('Child')])
        out.append(item)
        mismatch = copy.deepcopy(item)
        mismatch['service'] = 'Other'
        out.append(mismatch)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    cases = inputs()
    first = observe(args.rust, cases)
    assert same(first, observe(args.rust, cases)), 'Original runs differ'
    root = Path(__file__).resolve().parents[2]
    data = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                note='Original five typed save adapters and core EntityGraph conversion via access-only git archive bridges. Full returned GraphNode Debug, getters, provider/query/event sequence and errors are retained.',
                cases=[dict(input=i, expected=o) for i, o in zip(cases, first)])
    with gzip.GzipFile(Path(__file__).with_name('graph_save_adapter_cases.json.gz'), 'wb', mtime=0) as stream:
        stream.write(json.dumps(data, ensure_ascii=True, separators=(',', ':')).encode())
    print(f'Recorded {len(cases)} adapter observations from two matching original runs')


if __name__ == '__main__':
    main()
