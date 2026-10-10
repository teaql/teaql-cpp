"""Record original graph/ledger policy plans and their process-local execution sequence."""
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


def case(mode, **kwargs):
    out = dict(mode=mode, trace='trace', entity='Order', comment=None, root=True,
               items=[], loaded=None, ledger_comment=None, seed=[])
    out.update(kwargs)
    return out


def item(kind, values, fields=None, entity='Order', old=None):
    return dict(kind=kind, entity=entity, values=values, fields=fields or [], old=old)


def seed(identity, values=None, entity='Order', fresh=False, deleted=False, version=None):
    return dict(entity=entity, id=identity, values=values or {}, new=fresh,
                delete=deleted, version=version)


def inputs():
    identities = [None, v('null'), v('typed_null'), v('i64', '0'), v('i64', '-1'),
                  v('u64', '18446744073709551615'), v('text', '7'),
                  v('f64', '1.5'), v('decimal', '1.00'), v('bool', False),
                  v('json', {'z': 1}), v('object', {'id': v('u64', '7')})]
    versions = [None, v('null'), v('typed_null'), v('i64', '-7'), v('u64', '9'),
                v('u64', '18446744073709551615'), v('text', '8'),
                v('f64', '1.5'), v('decimal', '12.75')]
    fields = [[], ['name'], ['id', 'version', 'absent'], ['name', 'name', 'extra']]
    cases = [case('graph', root=False), case('graph', root=False,
               items=[item(0, {'id': v('u64', '1')})])]
    for kind, identity, version, selected in itertools.product(range(4), identities, versions, fields):
        values = dict(name=v('text', 'changed'), extra=v('object', {'deep': v('list', [v('text', 'x')])}))
        if identity is not None:
            values['id'] = identity
        if version is not None:
            values['version'] = version
        cases.append(case('graph', items=[item(kind, values, selected,
                     old={'version': v('i64', '99')})]))
    for trace, entity, comment in itertools.product(['', 'trace\x00tail', '\u03b1'],
                                                      ['', 'Order', 'Unicode\u2003'],
                                                      [None, '', 'audit']):
        cases.append(case('graph', trace=trace, entity=entity, comment=comment,
                     items=[item(k, {'id': v('u64', '2'), 'name': v('text', 'x')}, ['name'])
                            for k in [1, 3, 0, 2, 3]]))
    for identity, fresh, deleted, changed, version in itertools.product(
            [i for i in identities if i is not None], [False, True], [False, True],
            [False, True], [None, -7, 9]):
        cases.append(case('ledger', seed=[seed(identity,
                     {'name': v('text', 'after-delete')} if changed else {},
                     fresh=fresh, deleted=deleted, version=version)]))
    for root_comment, ledger_comment in itertools.product([None, '', 'root'], [None, '', 'ledger']):
        cases.append(case('ledger', comment=root_comment, ledger_comment=ledger_comment,
                     seed=[seed(v('u64', '2'), {'b': v('null')}, entity='Z'),
                           seed(v('i64', '1'), {'z': v('text', 'first')}),
                           seed(v('u64', '1'), {'a': v('text', 'merged')}),
                           seed(v('u64', '3'), fresh=True),
                           seed(v('u64', '4'), {'name': v('text', 'cancelled')},
                                fresh=True, deleted=True)]))
    for identity, version in itertools.product(identities[1:], versions):
        loaded = dict(id=v('u64', '7'))
        if version is not None:
            loaded['version'] = version
        cases.append(case('ledger', loaded=loaded,
                          seed=[seed(identity, {'name': v('text', 'update')})]))
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
                  note='Original private graph_policy_plan and ledger_policy_plan via access-only root exports; fixtures construct original models. Full execution IDs verify process-wide sequence ordering and that missing roots consume no sequence value.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('policy_plan_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} policy-plan observations')


if __name__ == '__main__':
    main()
