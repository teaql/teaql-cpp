"""Record original dependency sorting, Value matching, cycles and metadata errors."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import random
import subprocess
from compare_values import observe


def v(kind, value=None):
    out = dict(kind=kind)
    if value is not None:
        out['value'] = value
    return out


def key(entity, identity=None):
    return dict(entity=entity, id=identity if identity is not None else v('u64', '1'))


def relation(target, many=False, local='link', foreign='id', name='link'):
    return dict(name=name, target=target, many=many, local=local, foreign=foreign)


def case(keys, changes=None, descriptors=None):
    return dict(keys=keys, changes=changes or [], descriptors=descriptors if descriptors is not None
                else [dict(entity=name, relations=[]) for name in sorted({k['entity'] for k in keys})])


def inputs():
    cases = [case([])]
    variants = [v('null'), v('typed_null'), v('bool', False), v('bool', True),
                v('i64', '-1'), v('i64', '0'), v('i64', '1'), v('u64', '0'), v('u64', '1'),
                v('f64', '1'), v('text', '1'), v('decimal', '1.0'),
                v('date', '2026-10-06'), v('timestamp', '1791234000000'),
                v('json', {'deep': [1, 2]}), v('list', [v('u64', '1')]),
                v('object', {'id': v('u64', '1')})]
    for local, foreign, many, foreign_name in itertools.product(variants, variants,
                                                               [False, True], ['id', 'code']):
        keys = [key('A'), key('B')]
        changes = [dict(**keys[0], values={'link': local}),
                   dict(**keys[1], values={foreign_name: foreign})]
        descriptors = [dict(entity='A', relations=[relation('B', many, foreign=foreign_name)]),
                       dict(entity='B', relations=[])]
        cases.append(case(keys, changes, descriptors))
    # Absent target records and explicit NULL differ from an omitted foreign key.
    for many, foreign_name, target_values in itertools.product([False, True], ['id', 'code'],
                                                               [None, {}, {'id': v('null')}]):
        keys = [key('A'), key('B')]
        changes = [dict(**keys[0], values={'link': v('u64', '1')})]
        if target_values is not None:
            changes.append(dict(**keys[1], values=target_values))
        cases.append(case(keys, changes, [dict(entity='A', relations=[relation('B', many, foreign=foreign_name)]),
                                          dict(entity='B', relations=[])]))
    for permutation in itertools.permutations([key('A'), key('B'), key('C')]):
        cases.append(case(list(permutation)))
        cases.append(case(list(permutation), descriptors=[dict(entity='C', relations=[])]))
    # Canonical duplicate keys retain source insertion/collection behavior.
    for identities in itertools.permutations([v('i64', '1'), v('u64', '1'), v('i64', '-1')]):
        cases.append(case([key('A', identity) for identity in identities]))
    for values in [[v('null'), v('typed_null')], [v('typed_null'), v('null')],
                   [v('f64', '-0'), v('f64', '0')], [v('decimal', '1'), v('decimal', '1.0')]]:
        cases.append(case([key('A', value) for value in values]))
    for representatives, local, many in itertools.product(
            [[v('i64', '1'), v('u64', '1')], [v('u64', '1'), v('i64', '1')]],
            [v('i64', '1'), v('u64', '1')], [False, True]):
        keys = [key('A')] + [key('Z', identity) for identity in representatives]
        changes = [dict(**keys[0], values={'link': local})]
        descriptors = [dict(entity='A', relations=[relation('Z', many)]),
                       dict(entity='Z', relations=[])]
        cases.append(case(keys, changes, descriptors))
    rng = random.Random(505)
    for index in range(160):
        names = ['A', 'B', 'C', 'D', 'E'][:1 + index % 5]
        keys = [key(name) for name in names]
        descriptors = []
        changes = []
        for name in names:
            links = []
            values = {}
            for link_index in range(rng.randrange(4)):
                target = rng.choice(names)
                field = f'link{link_index}'
                links.append(relation(target, rng.choice([False, True]), field, name=field))
                if rng.randrange(5) != 0:
                    values[field] = rng.choice([v('u64', '1'), v('i64', '1'), v('null'), v('typed_null')])
                if index % 7 == 0:
                    links.append(copy.deepcopy(links[-1]))
            descriptors.append(dict(entity=name, relations=links))
            changes.append(dict(**key(name), values=values))
        rng.shuffle(keys)
        if index % 11 == 0:
            descriptors.pop()
        cases.append(case(keys, changes, descriptors))
    # Cycle diagnostics include blocked dependents, with unique sorted entities.
    keys = [key('A'), key('B'), key('C'), key('Z')]
    changes = [dict(**k, values={'link': v('u64', '1')}) for k in keys]
    descriptors = [dict(entity='A', relations=[relation('B'), relation('C', True)]),
                   dict(entity='B', relations=[relation('A')]),
                   dict(entity='C', relations=[]), dict(entity='Z', relations=[])]
    cases.append(case(keys, changes, descriptors))
    cases.append(case([key('A', identity) for identity in variants]))
    cases.append(case([key('A', identity) for identity in reversed(variants)]))
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
                  note='Original order_new_ledger_keys via access-only archive bridge; fixtures only construct metadata, keys and sparse changes.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('ledger_plan_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} ledger dependency observations')


if __name__ == '__main__':
    main()
