"""Record original mutation requests, conflict ordering, logs and audit delivery."""
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


def trace(kind=0, entity='Order', comment='audit'):
    return dict(kind=kind, entity=entity, comment=comment)


def case(**kwargs):
    out = dict(layer='entity', entry='update', entity='Order', metadata=True,
               required=False, fix=False, hook='none', policy='none', cache=True,
               log=False, counts=[1, 1], fail=-1, event_fail=-1,
               id=v('u64', '7'), version=3, values={'name': v('text', 'new')},
               old={'name': v('text', 'old'), 'other': v('text', 'retained')},
               trace=[trace(comment='command')], lineage=[trace(comment='lineage')],
               batch=[{'name': v('text', 'first'), 'extra': v('text', 'unselected')},
                      {'name': v('text', 'second')}],
               fields=['name'], ids=[v('u64', '7'), v('u64', '8')], versions=[3, 4],
               olds=[{'name': v('text', 'old')}, None], traces=[[], [trace(comment='second')]])
    out.update(kwargs)
    return out


def inputs():
    cases = []
    identities = [v('u64', '7'), v('i64', '-7'), v('text', '7'),
                  v('typed_null'), v('decimal', '7.0'), v('f64', '1.5')]
    for layer, entry, count, version, identity in itertools.product(
            ['base', 'context', 'entity'], ['insert', 'update', 'delete', 'recover'],
            [0, 1, 2], [None, 0, 3], identities):
        cases.append(case(layer=layer, entry=entry, counts=[count], version=version, id=identity))
    for entry, hook, policy, metadata, required, fix in itertools.product(
            ['insert', 'update', 'delete', 'recover'], ['none', 'set', 'error'],
            ['none', 'set', 'error'], [False, True], [False, True], [False, True]):
        cases.append(case(entry=entry, hook=hook, policy=policy, metadata=metadata,
                          required=required, fix=fix, values={}))
    for layer, entry, counts, shape in itertools.product(
            ['base', 'context', 'entity'], ['batch_insert', 'batch_update'],
            [[0, 0], [1, 1], [2, 1]], ['normal', 'empty', 'short_ids', 'extra_ids', 'no_versions',
                                      'short_versions', 'short_traces', 'short_old']):
        c = case(layer=layer, entry=entry, counts=counts)
        if shape == 'empty':
            c['batch'] = []
        elif shape == 'short_ids':
            c['ids'] = c['ids'][:1]
        elif shape == 'extra_ids':
            c['ids'].append(v('u64', '9'))
        elif shape == 'no_versions':
            c['versions'] = []
        elif shape == 'short_versions':
            c['versions'] = [None]
        elif shape == 'short_traces':
            c['traces'] = []
        elif shape == 'short_old':
            c['olds'] = []
        cases.append(c)
    for layer, entry, failure, event_failure, log in itertools.product(
            ['base', 'context', 'entity'], ['insert', 'update', 'delete', 'recover', 'batch_insert', 'batch_update'],
            [-1, 0, 1], [-1, 0, 1], [False, True]):
        cases.append(case(layer=layer, entry=entry, fail=failure, event_fail=event_failure, log=log))
    for entry, counts, failure in itertools.product(['inserts', 'updates', 'deletes', 'recovers'],
                                                   [[0, 0], [1, 1], [2, 1]], [-1, 0, 1]):
        cases.append(case(layer='base', entry=entry, counts=counts, fail=failure, log=True))
    for old, values in itertools.product([None, {}, {'name': v('text', 'old')},
                                          {'name': v('text', 'old'), 'extra': v('null')}],
                                         [{}, {'name': v('text', 'new')},
                                          {'name': v('text', 'new'), 'extra': v('null')} ]):
        cases.append(case(layer='prepared', old=old, values=values))
    for fields in [[], ['name', 'name'], ['missing'], ['name', 'extra']]:
        cases.append(case(entry='batch_update', fields=fields))
    for input_trace, lineage in itertools.product([[], [trace()], [trace(1)],
              [trace(1, 'Other'), trace(1)], [trace(1, 'Other')]], [[], [trace(comment='scope')]]):
        for entry in ['insert', 'update', 'delete', 'recover']:
            cases.append(case(layer='entity', entry=entry, trace=input_trace, lineage=lineage))
    for marker in [v('null'), v('typed_null'), v('u64', '0'), v('i64', '0'), v('bool', False)]:
        cases.append(case(entry='insert', values={'id': marker, 'version': marker}))
    cases.append(case(entry='insert', entity='Missing', hook='set', policy='set'))
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
                  note='Original base/context/entity mutation bodies via access-only archive bridges. Providers, hooks, checker, cache and event fixtures implement SPIs only. Covers partial batches, metadata before conflicts, success-only invalidation, old snapshot coverage, full versus SQL-projected event fields and trace asymmetries.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('runtime_mutation_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} runtime mutation observations')


if __name__ == '__main__':
    main()
