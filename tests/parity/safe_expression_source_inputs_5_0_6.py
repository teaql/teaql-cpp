"""Authored emptiness boundaries for each original TeaqlEmpty implementation."""

def cases():
    values = [{'kind': 'null'}, {'kind': 'typed_null'},
              {'kind': 'text', 'value': ''}, {'kind': 'text', 'value': 'a\0雪'},
              {'kind': 'bool', 'value': False}, {'kind': 'bool', 'value': True},
              {'kind': 'i64', 'value': '0'}, {'kind': 'u64', 'value': '18446744073709551615'},
              {'kind': 'f64', 'value': 'NaN'}, {'kind': 'f64', 'value': '-0'},
              {'kind': 'f64', 'value': 'inf'}, {'kind': 'decimal', 'value': '0.00'},
              {'kind': 'date', 'value': '2024-02-29'}, {'kind': 'timestamp', 'value': '0'},
              {'kind': 'list', 'value': []}, {'kind': 'list', 'value': [{'kind': 'null'}]},
              {'kind': 'object', 'value': {}},
              {'kind': 'object', 'value': {'empty': {'kind': 'text', 'value': ''}}}]
    values += [dict(kind='json', value=value) for value in
               [None, '', ' ', [], [None], {}, {'empty': None}, False, 0]]
    result = []
    for present in [False, True]:
        for kind in ['string', 'borrowed']:
            for index in range(8):
                result.append(dict(kind=kind, index=index, present=present))
        for items in [[], [''], ['', ''], [' '], ['雪'], ['a\0b', 'x' * 384]]:
            result.append(dict(kind='vector', items=items, present=present))
        for items in [[], [values[0]], [values[2]], [values[14]]]:
            for loaded in [False, True]:
                result.append(dict(kind='smart_list', items=items, loaded=loaded, present=present))
        result.append(dict(kind='option', some=False, value=values[0], present=present))
        for index in [0, 1, 2, 4, 6, 14, 16, 18]:
            result.append(dict(kind='option', some=True, value=values[index], present=present))
        for value in values:
            result.append(dict(kind='value', value=value, present=present))
    return result
