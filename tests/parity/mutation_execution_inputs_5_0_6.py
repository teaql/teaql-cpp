"""Inputs for original mutation orchestration; no execution or privacy algorithms."""
from copy import deepcopy
from itertools import product


def scalar(kind, value):
    return dict(kind=kind, value=value)


ENTITY = dict(name='Account', table='order', declared=True, masked=['name'], properties=[
    dict(name='id', type=2, id=True, column='record_id'),
    dict(name='version', type=1, version=True, column='revision'),
    dict(name='name', type=5, column='display name'),
    dict(name='token', type=5, column='access_token'),
    dict(name='status', type=5), dict(name='tenant', type=2),
])
GUARD = dict(kind='binary', left=dict(kind='column', name='tenant'), op=0,
             right=dict(kind='value', value=scalar('u64', '7')))
STEPS = (
    [], [dict(count=0)], [dict(count=2)], [dict(count=1), dict(count=0)],
    [dict(count=1), dict(count=2)], [dict(mode='error')],
    [dict(count=1), dict(mode='error')], [dict(mode='pending')],
    [dict(count=1), dict(mode='pending')], [dict(mode='panic')],
    [dict(count=1), dict(mode='panic')], [dict(count=1), dict(count=1)],
)


def leaf(kind, reason='Change the account', identity='1'):
    command = dict(mode='delete' if kind == 'hard_delete' else kind,
                   comment=reason, id=scalar('u64', identity), version='-3' if kind == 'recover' else '1',
                   values=dict(name=scalar('text', 'Riverside')),
                   trace_nodes=[dict(kind=8, entity='Account', id=int(identity), comment=reason)])
    if kind == 'insert':
        command['values'].update(id=scalar('u64', identity), version=scalar('i64', '1'))
    if kind == 'update':
        command['old_values'] = dict(name=scalar('text', 'OLD-NAME'),
                                     password=scalar('text', 'OLD-CREDENTIAL'))
    if kind in ('delete', 'hard_delete'):
        command['soft'] = kind == 'delete'
    return command


def attach(request, variant):
    if variant:
        request['trusted_redactions'] = [scalar('text', 'SIDECAR-CANARY')]
    if variant == 2:
        request['trusted_redactions'] += [scalar('text', 'SIDECAR-CANARY'), scalar('u64', '99')]
    return request


def cases():
    result = []
    for kind, guarded, transaction, observe, sidecar, steps in product(
            ('insert', 'update', 'delete', 'hard_delete', 'recover'),
            (False, True), (False, True), (False, True), range(3), STEPS):
        row = dict(entity=deepcopy(ENTITY), request=attach(leaf(kind), sidecar),
                   transaction=transaction, observe=observe, steps=deepcopy(steps))
        if guarded:
            row['guard'] = deepcopy(GUARD)
        result.append(row)
    for shape, transaction, observe, sidecar, steps in product(
            range(8), (False, True), (False, True), range(3), STEPS[:6]):
        child = leaf('insert', 'ROOT' if shape == 3 else 'INNER' if shape == 4 else 'ITEM')
        children = [child]
        if shape == 0:
            children = []
        if shape >= 2:
            children = [dict(mode='batch', comment='ROOT' if shape == 2 else 'INNER', children=children)]
        if shape >= 5:
            sibling = leaf('update', 'FUTURE OLD-NAME OLD-CREDENTIAL', '2')
            if shape == 6:
                sibling['entity'] = 'Missing'
            if shape == 7:
                sibling['values']['unrecognized'] = scalar('text', 'UNKNOWN-FIELD')
                sibling['trusted_redactions'] = [scalar('text', 'FUTURE-SIDECAR')]
            children.append(sibling)
        row = dict(entity=deepcopy(ENTITY), transaction=transaction, observe=observe,
                   request=attach(dict(mode='batch', comment='ROOT', children=children), sidecar),
                   steps=deepcopy(steps))
        result.append(row)
    assert len(result) == 2016
    return result


def cache_cases():
    result = []
    for masked, prewarm, count, transaction in product((False, True), (False, True), range(3), (False, True)):
        row = dict(entity=deepcopy(ENTITY), masked=masked, prewarm=prewarm, transaction=transaction,
                   request=dict(mode='batch', comment='Inspect cached metadata',
                                children=[leaf('update', 'Change cached account', str(i + 1)) for i in range(count)]))
        result.append(row)
    assert len(result) == 24
    return result
