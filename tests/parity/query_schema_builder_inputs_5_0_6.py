"""Readonly schema replacement, sharing and query-state boundaries."""
import itertools


def cases():
    result = []
    for kind, flags in itertools.product(range(10), range(8)):
        first = [['_rank', kind]]
        second = [['_rank', (kind + 1) % 10]]
        shapes = [([], []), ([], first), (first, []), (first, first),
                  (first, second),
                  ([['_雪\0name', kind], ['_' + 'x' * 256, 5]],
                   [['_雪\0name', (kind + 1) % 10], ['_other', 7]])]
        for left, right in shapes:
            result.append(dict(first=left, second=right, flags=flags))
    return result
