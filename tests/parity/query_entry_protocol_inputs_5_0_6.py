"""Preserve aggregate controls and compare distinct query entry protocols."""
import itertools

from enhanced_aggregate_inputs_5_0_6 import inputs as retained_inputs


def inputs():
    result = retained_inputs()
    entries = ['enhanced', 'aggregates', 'owned', 'rows', 'rows_owned', 'rows_aggregates',
               'list', 'list_aggregates', 'typed', 'typed_aggregates']
    argument_entries = {'aggregates', 'owned', 'rows_aggregates', 'list_aggregates', 'typed_aggregates'}
    for entry in entries:
        placements = ['none', 'inline', 'separate', 'both'] if entry in argument_entries else ['none', 'inline']
        for placement, single, missing, empty, failure, alias, feature in itertools.product(
                placements, [False, True], [False, True], [False, True], [False, True],
                ['total', 'id'], ['ordinary', 'projection', 'continuous_first', 'continuous_miss',
                                  'id_set', 'dynamic']):
            result.append(dict(entry=entry, placement=placement, single=single, missing=missing,
                               empty=empty, failure=failure, alias=alias, feature=feature, protocol=True))
    assert len(result) == 6080
    return result
