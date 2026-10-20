"""Query-owned and separately transported relation aggregate declarations."""
import itertools


def inputs():
    rows = []
    for entry in ['enhanced', 'aggregates', 'owned']:
        placements = ['none', 'inline'] if entry == 'enhanced' else ['none', 'inline', 'separate', 'both']
        for placement, single, missing, empty, failure, alias in itertools.product(
                placements, [False, True], [False, True], [False, True],
                [False, True], ['total', 'id']):
            rows.append(dict(entry=entry, placement=placement, single=single, missing=missing,
                             empty=empty, failure=failure, alias=alias))
    assert len(rows) == 320
    return rows
