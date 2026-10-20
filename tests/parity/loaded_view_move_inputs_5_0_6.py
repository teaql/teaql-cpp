"""Complete weak/copy/ledger/load/publication matrix for consuming view transfer."""
from itertools import product


def cases():
    names = ('weak', 'copy', 'receiver_active', 'source_active', 'frozen',
             'receiver_load', 'source_load')
    return [dict(zip(names, row)) for row in product(*([[False, True]] * 5 + [range(4)] * 2))]
