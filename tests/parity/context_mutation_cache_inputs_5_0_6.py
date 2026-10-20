"""Independent controls for Context mutation/cache observations, without expectations."""
import itertools


def inputs():
    rows = []
    for entry in ['insert', 'update', 'delete', 'recover', 'batch_insert', 'batch_update']:
        for length, intent, versions, counts, failure, caches in itertools.product(
                range(3) if entry.startswith('batch_') else [1], [False, True],
                [[], [3], [None, None, 3]], [[], [0, 1], [1, 0], [1, 1]], [-1, 0, 1], range(4)):
            for ids in ([max(length - 1, 0), length, length + 1] if entry == 'batch_update' else [length]):
                rows.append(dict(entry=entry, length=length, ids=ids, intent=intent,
                                 versions=versions, counts=counts, failure=failure, caches=caches))
    return rows
