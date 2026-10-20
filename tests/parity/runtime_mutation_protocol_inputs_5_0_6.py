"""Fixture controls for actual base mutations; no runtime expectations are encoded."""
import itertools


def inputs():
    maximum = 2**64 - 1
    identities = [dict(kind='u64', value='7'), dict(kind='i64', value='-1'),
                  dict(kind='text', value='external'), dict(kind='null'),
                  dict(kind='u64', value='0')]
    rows = []

    def add(entry, identity, length, ids, versions, intent, counts, observer, event, failure):
        index = len(rows)
        rows.append(dict(entry=entry, id=identity, length=length, ids=ids,
                         versions=versions, intent=intent, counts=counts,
                         observer=observer, event=event, failure=failure,
                         trace=bool(index % 2), old=bool((index // 4) % 2),
                         privacy=intent and bool((index // 2) % 2)))

    for entry, identity, intent, versions, (counts, failure), (observer, event) in itertools.product(
            ['insert', 'update', 'delete', 'recover'], identities, [False, True],
            [[], [None], [3]], [([0], -1), ([1], -1), ([maximum], -1), ([1], 0)],
            itertools.product([False, True], repeat=2)):
        add(entry, identity, 1, 1, versions, intent, counts, observer, event, failure)

    counts = [[], [0, 1], [1, 0], [1, 1], [maximum, 1], [maximum, 0]]
    delivery = [(False, False, -1), (True, True, -1), (True, False, 0), (True, True, 1)]
    for entry, length, intent, versions, affected, (observer, event, failure) in itertools.product(
            ['inserts', 'updates', 'deletes', 'recovers'], range(3), [False, True],
            [[], [3]], counts, delivery):
        add(entry, identities[0], length, length, versions, intent, affected, observer, event, failure)

    for entry, length, ids, intent, versions, affected, (observer, event, failure) in itertools.product(
            ['batch_insert', 'batch_update'], range(3), range(4), [False, True],
            [[], [None], [3], [None, 4], [None, None, 5]], counts, delivery):
        add(entry, identities[0], length, ids, versions, intent, affected, observer, event, failure)
    return rows
