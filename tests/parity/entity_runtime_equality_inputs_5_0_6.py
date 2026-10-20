"""Exact floating bit patterns cross absent payloads and cold/materialized ledgers."""
from itertools import product

BITS = (None, '0000000000000000', '8000000000000000', '3ff0000000000000',
        'bff0000000000000', '0000000000000001', '7fefffffffffffff',
        '7ff0000000000000', 'fff0000000000000', '7ff8000000000000',
        '7ff8000000000042', '7ff0000000000001', 'fff8000000000042')


def cases():
    return [dict(bits=left, other_bits=right, active=active)
            for left, right, active in product(BITS, BITS, (False, True))]
