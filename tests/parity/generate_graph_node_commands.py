"""Record original private graph node commands through access-only bridges."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same
from generate_graph_execution import case, node, descriptor, v


def inputs():
    cases = []
    identities = [None, v('null'), v('u64', '0'), v('i64', '0'), v('typed_null'),
                  v('u64', '7'), v('i64', '-7'), v('text', 'identity🧊'), v('u64', '18446744073709551615')]
    versions = [None, v('i64', '-1'), v('i64', '0'), v('i64', '1'), v('i64', '2'),
                v('u64', '1'), v('text', '1')]
    for mode, identity, stored, expected in itertools.product(['node_reference', 'node_remove'], identities, versions, versions):
        values = {} if identity is None else {'id': identity}
        if expected is not None:
            values['version'] = expected
        row = {'id': v('u64', '7'), 'name': v('text', 'authoritative allocated payload')}
        if stored is not None:
            row['version'] = stored
        item = case(mode=mode, root=node(values=values, comment='discarded annotation'), rows=[row],
                    trace=[dict(kind=0, entity='Ancestor', comment='parent'), dict(kind=1, entity='Order', comment='')])
        item['root'].update(dirty=['id'], old={'version': v('i64', '5')})
        cases.append(item)
    for mode, shape in itertools.product(['node_reference', 'node_remove', 'node_delete'], range(8)):
        item = case(mode=mode, root=node(values={'id': v('u64', '7'), 'version': v('i64', '1')}))
        if shape == 0:
            item['root']['relations']['empty'] = []
        elif shape == 1:
            item['root']['values']['name'] = v('text', 'mutable field')
        elif shape == 2:
            item['descriptors'] = []
        elif shape == 3:
            item['descriptors'] = [descriptor(identity='')]
        elif shape == 4:
            item['rows'] = []
        elif shape == 5:
            item['read_fail'] = True
        elif shape == 6:
            item['fail'] = 0
        else:
            item['event_fail'] = 0
        cases.append(item)
    for identity, expected, comment, parent in itertools.product(identities, versions, [None, '', 'delete🧊'], [False, True]):
        values = {} if identity is None else {'id': identity}
        if expected is not None:
            values['version'] = expected
        cases.append(case(mode='node_delete', root=node(values=values, comment=comment),
                          trace=[dict(kind=0, entity='Ancestor', comment='parent')] if parent else []))
    for checker, dirty, version, shape in itertools.product(['none', 'fill', 'erase', 'error'], [None, [], ['name'], ['version', 'id']], versions, range(3)):
        values = {'id': v('u64', '7'), 'name': v('text', 'unchanged input'),
                  'unknown': v('text', 'ignored projection'), '__teaql_object_status': v('text', 'corrupt marker')}
        if version is not None:
            values['version'] = version
        if shape == 1:
            values.pop('name')
        root = node(values=values, comment='retained annotation')
        root.update(dirty=dirty, old={'name': v('text', 'original allocated snapshot')})
        cases.append(case(mode='node_update', root=root, checker=checker,
                          descriptors=[] if shape == 2 else [descriptor()]))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    cases = inputs()
    expected = observe(args.rust, cases)
    assert same(expected, observe(args.rust, cases)), 'Original runs differ'
    root = Path(__file__).resolve().parents[2]
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original reference/remove validation, checker update command and scoped deletion through access-only archive bridges. Full model/command outputs, provider/query/event calls, error and marker cleanup are retained.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, expected)])
    with gzip.GzipFile(Path(__file__).with_name('graph_node_command_cases.json.gz'), 'wb', mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(',', ':')).encode())
    print(f'Recorded {len(cases)} graph node command observations from two matching original runs')


if __name__ == '__main__':
    main()
