"""Compare scalar aggregate membership with object-valued query projections."""
import itertools

from query_entry_cursor_inputs_5_0_6 import inputs as retained_inputs


def inputs():
    rows = retained_inputs()
    arguments = {'aggregates', 'owned', 'rows_aggregates', 'list_aggregates', 'typed_aggregates'}
    for entry in ['enhanced', 'aggregates', 'owned', 'rows', 'rows_owned', 'rows_aggregates',
                  'list', 'list_aggregates', 'typed', 'typed_aggregates']:
        placements = ['inline', 'separate', 'both'] if entry in arguments else ['inline']
        for placement, single, missing, empty, failure, alias, group, feature in itertools.product(
                placements, [False, True], [False, True], [False, True], [False, True],
                ['total', 'bucket'], ['overwrite', 'separate'], ['ordinary', 'continuous_first']):
            rows.append(dict(entry=entry, placement=placement, single=single, missing=missing,
                             empty=empty, failure=failure, alias=alias, group=group, feature=feature,
                             membership=True, protocol=True,
                             **({'cursor_probe': True} if feature == 'continuous_first' else {})))
    assert len(rows) == 10560
    return rows
