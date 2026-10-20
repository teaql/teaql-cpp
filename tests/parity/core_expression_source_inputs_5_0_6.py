"""Authored boundaries for actual Expr calls; no expression implementation."""

def api_cases():
    texts = ['', '\0tail', '雪😀', '%_\\\'"', '\n\r\t', 'e\u0301', 'x' * 384, 'a\0b']
    values = [{'kind': 'null'}, {'kind': 'typed_null'}, {'kind': 'f64', 'value': 'NaN'},
              {'kind': 'f64', 'value': '-0'}, {'kind': 'u64', 'value': '18446744073709551615'},
              {'kind': 'text', 'value': 'a\0b'}, {'kind': 'list', 'value': []},
              {'kind': 'object', 'value': {'雪': {'kind': 'f64', 'value': 'NaN'}}}]
    result = []
    for i, column in enumerate(texts):
        for j, pattern in enumerate(texts):
            result.append(dict(kind='full_expression', column=column, pattern=pattern,
                               lower=values[(i+j) % len(values)],
                               upper=values[(i+j+1) % len(values)], filter=(i+j) % 2 == 0))
    for function in range(14):
        for count in [0, 1, 19, 20, 21, 22, 41]:
            result.append(dict(entity='Boundary\0雪', projections=[[], ['a', 'a', '']],
                               expires='0', propagate='0', function=function,
                               values=[values[(index // 2) % len(values)] for index in range(count)]))
    return result


def like_cases():
    columns = ['', 'quoted"name', 'A\0B', '雪😀']
    texts = ['', '%_\\\'"', 'a\0b', '\n\r\t', 'e\u0301', 'x' * 384, '\u2003', '雪😀']
    return [dict(column=column, text=text) for column in columns for text in texts]
