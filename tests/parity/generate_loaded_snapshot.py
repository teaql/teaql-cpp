#!/usr/bin/env python3
"""Record original 5.0.6 snapshot/property calls after exact C++ comparison."""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
from generate_field_layout import observe

ROOT = Path(__file__).resolve().parents[2]


def cases():
    out = []
    for count in [2, 31, 32, 63, 64, 65, 66, 129, 130, 141]:
        lookups = ['id', 'version', 'field_31', 'field_63', 'field_64', 'field_65',
                   'field_129', 'lines', '#custom', '_total', 'missing']
        for full in [False, True]:
            for selected in [[], ['id'], ['field_63', 'field_64', 'field_129'],
                             ['id', 'lines', '#custom', '_total', 'missing']]:
                for schema in [None, [], [['_total', 1], ['_text', 5]]]:
                    out.append(dict(kind='snapshot', count=count, full=full, selected=selected,
                                    actions=[['id', True], ['id', False], ['id', True],
                                             ['field_64', True], ['field_64', False],
                                             ['lines', True], ['lines', False],
                                             ['#custom', True], ['#custom', False],
                                             ['_total', True], ['missing', False]],
                                    schema=schema, lookups=lookups))
    for count in [2, 64, 65, 141]:
        for mode in ['not_loaded', 'partial', 'partial_compact', 'shared_columns', 'full', 'indexed']:
            for different in [False, True]:
                out.append(dict(kind='load_state', count=count, mode=mode,
                    selected=['id', 'field_64', 'lines', '#custom', '_total', 'missing'],
                    actions=[['id', False], ['version', True], ['field_64', False],
                             ['lines', False], ['#custom', True], ['_total', True], ['missing', False]],
                    different_layout=different,
                    lookups=['id', 'version', 'field_63', 'field_64', 'lines', '#custom', '_total', 'missing']))
    for count in [2, 31, 32, 63, 64, 65, 66, 129, 130, 141]:
        for selected in [[], ['id'], ['version', 'id'], ['field_63', 'field_64', 'field_129'],
                         ['id', 'lines', '#custom', '_total', 'missing'], ['id', 'version']]:
            out.append(dict(kind='row', count=count, selected=selected,
                            lookups=['id', 'version', 'field_31', 'field_63', 'field_64',
                                     'field_129', 'lines', '#custom', '_total', 'missing']))
    for count in [2, 64, 65, 141]:
        for selected in [[], ['id'], ['id', 'version'], ['field_64', 'field_129']]:
            out.append(dict(kind='row', count=count, selected=selected, relation_loaded=True,
                            lookups=['id', 'version', 'field_64', 'field_129', 'lines']))
    values = [dict(kind='null'), dict(kind='bool', value=False), dict(kind='bool', value=True),
              dict(kind='i64', value='0'), dict(kind='i64', value='-1'),
              dict(kind='u64', value='0'), dict(kind='u64', value=str((1 << 64) - 1)),
              dict(kind='f64', value='1.0'), dict(kind='decimal', value='12.5'),
              dict(kind='text', value=''), dict(kind='text', value='readonly'),
              dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='123'),
              dict(kind='list', value=[]), dict(kind='object', value={})]
    values += [dict(kind='typed_null', datatype=i) for i in range(10)]
    values += [dict(kind='json', value=v) for v in [None, False, True, 0, -1, (1 << 63)-1,
               1 << 63, (1 << 64)-1, 1.0, 0.125, '', 'text', [], {}, [1], {'a': 1}]]
    for datatype in range(10):
        validations = [dict(name='_value', input=v) for v in values]
        validations += [dict(name='_unknown', input=v) for v in values]
        for merged in [[], [['_value', datatype]], [['_value', datatype], ['_other', 5]],
                       [['_value', (datatype+1) % 10]], [['_other', 5]]]:
            out.append(dict(kind='schema', fields=[['_value', datatype]], merge=merged,
                            values=validations))
    for fields in [[], [['bad', 5]], [['#bad', 5]], [['_', 5]], [['_ ', 5]],
                   [['_\u00a0', 5]], [['_\u3000', 5]], [['_value', 5], ['_value', 5]],
                   [['_value', 5], ['_value', 1]], [['_雪', 5]], [['_first', 0], ['_last', 7]]]:
        out.append(dict(kind='schema', fields=fields, merge=[], values=[]))
    base = dict(kind='snapshot', count=65, full=False, selected=['id'], actions=[],
                schema=None, lookups=['id', 'lines', '#custom', '_value'])
    for invalid in [[['bad', 1]], [['_value', 1], ['_value', 1]]]:
        c = deepcopy(base)
        c['schema'] = invalid
        out.append(c)
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rust', type=Path, required=True)
    p.add_argument('--cpp', type=Path, required=True)
    args = p.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected, actual = observe(args.rust.resolve(), inputs), observe(args.cpp.resolve(), inputs)
    for i, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            raise SystemExit(f'Snapshot/schema source mismatch {i}:\n{left}\n{right}\n{inputs[i]}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  note='Original LoadedSnapshot fixed/overflow/named geometry, immutable updates and readonly schema/type/merge calls. Published source snapshots are held for Context-lifetime equivalence. Excludes durable field values, generated hydration and complete source cache-lifetime qualification.',
                  cases=[dict(input=i, expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('loaded_snapshot_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2)+'\n')
    print(f'Compared and recorded {len(inputs)} direct Rust snapshot/schema inputs')


if __name__ == '__main__':
    main()
