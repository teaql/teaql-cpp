"""Record original registered graph/ledger savers and transaction/error ordering."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
from generate_graph_execution import node, v
from generate_ledger_execution import descriptor, seed


def case(**kwargs):
    result = dict(mode='graph', root=node(operation=1, comment='save'),
                  seeds=[seed(fresh=True)], descriptors=[descriptor(), descriptor('Child')],
                  rows=[{'id': v('u64', '7'), 'version': v('i64', '9'), 'name': v('text', 'database')}],
                  ambient_rows=[], read_batches=[], counts=[1, 1], read_fail_at=-1,
                  fail=-1, event_fail=-1, begin_fail=False, commit_fail=False, rollback_fail=False,
                  ledger_comment='ledger', policy='none')
    result.update(kwargs)
    return result


def inputs():
    cases = []
    for mode, operation, policy, ambient, rows, begin_fail, commit_fail, rollback_fail in itertools.product(
            ['graph', 'ledger'], range(4), ['none', 'allow', 'deny'],
            [[], [{'id': v('u64', '7'), 'version': v('i64', '3')}]],
            [[], [{'id': v('u64', '7'), 'name': v('text', 'triggered')}]],
            [False, True], [False, True], [False, True]):
        cases.append(case(mode=mode, root=node(operation=operation), policy=policy,
                          ambient_rows=ambient, rows=rows, begin_fail=begin_fail,
                          commit_fail=commit_fail, rollback_fail=rollback_fail))
    for mode, failure, event_failure, read_failure, rollback_failure in itertools.product(
            ['graph', 'ledger'], [-1, 0], [-1, 0], [-1, 0, 1], [False, True]):
        cases.append(case(mode=mode, fail=failure, event_fail=event_failure,
                          read_fail_at=read_failure, rollback_fail=rollback_failure))
    for fresh, removed, values, version, rows, committed in itertools.product(
            [False, True], [False, True], [{}, {'name': v('text', 'changed')}],
            [None, 3], [[], [{'id': v('u64', '7')}]], [False, True]):
        cases.append(case(mode='ledger', seeds=[seed(fresh=fresh, delete=removed, values=values, version=version)],
                          rows=rows, commit_fail=committed))
    for identity, values, comment, failed_commit in itertools.product(
            [v('u64', '0'), v('i64', '0'), v('null'), v('typed_null')],
            [{}, {'id': v('u64', '0')}], [None, '', 'reason'], [False, True]):
        cases.append(case(mode='ledger', root=node(values=values, comment=comment),
                          seeds=[seed(identity=identity, fresh=True)],
                          commit_fail=failed_commit, ledger_comment=comment))
    for mode, descriptors, values in itertools.product(['graph','ledger'],
            [[], [descriptor(identity='')], [descriptor(identity='pk')], [descriptor(required=True)]],
            [{}, {'name': v('null')}, {'pk': v('u64', '7')} ]):
        cases.append(case(mode=mode, root=node(values=values), descriptors=descriptors,
                          seeds=[seed(values=values,fresh=True)]))
    for comment in [None, '', 'why']:
        for mode in ['graph','ledger']:
            cases.append(case(mode=mode, root=node(comment=comment,
                         relations={'children':[node('Child', {'id':v('u64','8'), 'name':v('text','unchanged child')}, operation=2)]}),
                         ledger_comment=comment))
    cases.append(case(mode='ledger', seeds=[], root=node(values={})))
    cases.append(case(mode='ledger', seeds=[seed(delete=True,version=3),seed('Child',fresh=True)],
                      read_batches=[[], [{'id':v('u64','7')}]]))
    return cases


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--rust',required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    cases=inputs()
    outputs=observe(args.rust,cases)
    lock=json.loads((root/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],
                note='Original GraphSaverFor graph/ledger methods via registered DynGraphSaver, with access-only locked archive bridges. Fixtures replace provider/transaction/policy/event SPIs only. Observations include original pending-ledger state after commit/failure.',
                cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    Path(__file__).with_name('graph_saver_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Recorded {len(cases)} transactional saver observations')


if __name__=='__main__':main()
