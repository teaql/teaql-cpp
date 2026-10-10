"""Record original ledger planning, payload gates, writes and audit call sites."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def v(kind, value=None):
    result = dict(kind=kind)
    if value is not None:
        result['value'] = value
    return result


def seed(entity='Order', identity=None, values=None, fresh=False, delete=False, version=None, path=None):
    return dict(entity=entity, id=identity if identity is not None else v('u64', '7'),
                values={'name': v('text', 'new')} if values is None else values,
                new=fresh, delete=delete, version=version, path=path or [])


def descriptor(entity='Order', identity='id', version='version', required=False, timestamps=False, relations=None):
    return dict(entity=entity, id=identity, version=version, required=required,
                timestamps=timestamps, relations=relations or [])


def case(**kwargs):
    result = dict(service='Order', seeds=[seed()], descriptors=[descriptor(), descriptor('Child')],
                  rows=[], read_batches=[], read_fail_at=-1, counts=[1, 1, 1],
                  fail=-1, event_fail=-1, id_fail=-1, loaded=None, comment=None,
                  composition_conflict=False)
    result.update(kwargs)
    return result


def inputs():
    cases = [case(seeds=[]), case(composition_conflict=True)]
    records = [{}, {'name': v('text', 'new')}, {'name': v('null')}, {'name': v('typed_null')},
               {'name': v('text', '')}, {'id': v('u64', '99'), 'name': v('text', 'override')},
               {'version': v('i64', '0')}, {'version': v('u64', '5')},
               {'version': v('typed_null')}, {'x,y': v('text', 'comma')}, {'': v('bool', True)}]
    for values, fresh, removed, rows, version, required in itertools.product(
            records, [False, True], [False, True], [[], [{'id': v('u64', '7')}]],
            [None, 0, 3], [False, True]):
        cases.append(case(seeds=[seed(values=values, fresh=fresh, delete=removed, version=version,
                                     path=['items', 0])], rows=rows,
                          descriptors=[descriptor(required=required)]))
    identities = [v('null'), v('typed_null'), v('i64', '0'), v('u64', '0'), v('i64', '-1'),
                  v('u64', '17'), v('text', '17'), v('bool', False), v('decimal', '0')]
    for identity, fresh, rows, id_fail in itertools.product(identities, [False, True],
                                                        [[], [{'id': v('u64', '7')}]], [-1, 0]):
        cases.append(case(seeds=[seed(identity=identity, fresh=fresh, version=1)], rows=rows, id_fail=id_fail))
    for values, fresh, rows in itertools.product(
            [{'name': v('text', 'clock')}, {'create_time': v('timestamp', '17'), 'update_time': v('timestamp', '19')},
             {'create_time': v('typed_null'), 'update_time': v('typed_null')},
             {'create_time': v('i64', '0'), 'update_time': v('u64', '0')}], [False, True],
            [[], [{'id': v('u64', '7')}]]):
        cases.append(case(seeds=[seed(values=values, fresh=fresh, version=1)], rows=rows,
                          descriptors=[descriptor(timestamps=True)]))
    for identity, version, values, rows in itertools.product(['', 'id', 'pk'], ['', 'version', 'revision'],
            [{'name': v('text', 'shape')}, {'pk': v('u64', '7'), 'version': v('i64', '5')}],
            [[], [{'pk': v('u64', '7')}]]):
        cases.append(case(seeds=[seed(values=values, version=3)], rows=rows,
                          descriptors=[descriptor(identity=identity, version=version)]))
    for comment in [None, '', 'reason']:
        for original in [None, 2]:
            cases.append(case(comment=comment, seeds=[seed(version=original)],
                              loaded={'id': v('u64', '7'), 'version': v('i64', '5')},
                              rows=[{'id': v('u64', '7')}]))
    # Partial failure and event ordering for each execution phase.
    for phase, fail, event_fail, count in itertools.product(['insert', 'update', 'delete'],
                                                           [-1, 0, 1], [-1, 0, 1], [0, 1, 2]):
        seeds = [seed(identity=v('u64', str(i)), fresh=phase == 'insert', delete=phase == 'delete', version=1)
                 for i in [7, 8]]
        cases.append(case(seeds=seeds, fail=fail, event_fail=event_fail, counts=[count, count],
                          rows=[{'id': v('u64', '7')}] if phase == 'update' else [], comment='write'))
    # Whole-graph preflight must precede even an unrelated existing deletion.
    for missing in [False, True]:
        cases.append(case(seeds=[seed(delete=True, version=2),
                                seed('Child', v('u64', '0'), {'name': v('null') if missing else v('text', 'child')}, fresh=True)],
                          descriptors=[descriptor(), descriptor('Child', required=True)], comment='gate'))
    for read_failure in [0, 1, 2]:
        cases.append(case(seeds=[seed(), seed(identity=v('u64', '8'))], read_fail_at=read_failure))
    for rows in [[[], [{'id': v('u64', '7')}]], [[{'id': v('u64', '7')}], []]]:
        cases.append(case(seeds=[seed(version=1), seed(identity=v('u64', '8'), version=2)],
                          read_batches=rows))
    for descriptors in [[], [descriptor('Child')], [descriptor()]]:
        cases.append(case(seeds=[seed(fresh=True), seed('Child', fresh=True)], descriptors=descriptors))
    # Dependency direction, cycles, adjacent grouping, and generated IDs are source behavior.
    for many, cyclic in itertools.product([False, True], [False, True]):
        def relation(target, name='link'):
            return dict(name=name, target=target, local='link', foreign='id', many=many)
        descriptors = [descriptor('A', relations=[relation('Z')]),
                       descriptor('Z', relations=[relation('A')] if cyclic else [])]
        seeds = [seed('A', v('u64', '1'), {'name': v('text', 'a'), 'link': v('u64', '2')}, fresh=True),
                 seed('Z', v('u64', '2'), {'name': v('text', 'z'), 'link': v('u64', '1')}, fresh=True)]
        cases.append(case(seeds=seeds, descriptors=descriptors))
    for failure in [-1, 0, 1]:
        cases.append(case(seeds=[seed('Order', v('u64', '0'), fresh=True),
                                seed('Child', v('null'), fresh=True)], id_fail=failure))
    # Nested Value payloads and independent update signatures.
    for fresh in [False, True]:
        cases.append(case(seeds=[seed(values={'a': v('object', {'deep': v('list', [v('text', 'nested')])})}, fresh=fresh, version=2),
                                seed(identity=v('u64', '8'), values={'b': v('decimal', '1.23')}, fresh=fresh, version=3)],
                          rows=[{'id': v('u64', '7')}], comment='nested'))
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
                  note='Original execute_ledger_plan_internal via access-only archive bridge; provider/sink/ID fixtures only. Timestamp values above 1000000 are normalized to Timestamp(<now>); fixed timestamps remain exact.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('ledger_execution_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} ledger execution observations')


if __name__ == '__main__':
    main()
