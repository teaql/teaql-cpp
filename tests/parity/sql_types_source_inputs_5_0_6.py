"""Authored SQLite renderer/callback/value/error boundaries, native64 model widths."""
from copy import deepcopy


def cases():
    null = {'kind': 'null'}
    number = {'kind': 'i64', 'value': '7'}
    nan = {'kind': 'f64', 'value': 'NaN'}
    values = [null, {'kind': 'typed_null'}, {'kind': 'bool', 'value': False},
              {'kind': 'bool', 'value': True}, {'kind': 'i64', 'value': '-9223372036854775808'},
              {'kind': 'u64', 'value': '18446744073709551615'}, nan,
              {'kind': 'f64', 'value': 'inf'}, {'kind': 'f64', 'value': '-inf'},
              {'kind': 'f64', 'value': '-0'}, {'kind': 'decimal', 'value': '1.2300'},
              {'kind': 'text', 'value': "a'\0雪\\"}, {'kind': 'json', 'value': {'雪': "a'\0"}},
              {'kind': 'date', 'value': '2024-02-29'},
              {'kind': 'timestamp', 'value': '-9223372036854775808'},
              {'kind': 'timestamp', 'value': '9223372036854775807'},
              {'kind': 'object', 'value': {'n': nan}},
              {'kind': 'object', 'value': {'list': {'kind': 'list', 'value': [nan]}}},
              {'kind': 'list', 'value': [nan]},
              {'kind': 'list', 'value': [{'kind': 'list', 'value': [nan]}]},
              {'kind': 'list', 'value': [{'kind': 'object', 'value': {'n': nan}}]},
              {'kind': 'list', 'value': []}, {'kind': 'object', 'value': {}},
              {'kind': 'list', 'value': [number, null, {'kind': 'typed_null'}]}]
    result = [dict(kind='value', value=value) for value in values]
    templates = ['', 'SELECT 1', 'SELECT ?', 'SELECT ?, ?', 'SELECT ?, ?, ?', 'SELECT ??',
                 'SELECT ?0', 'SELECT ?1', 'SELECT ?9999999999999999999999',
                 'SELECT :name', 'SELECT @name', 'SELECT $name', 'SELECT :', 'SELECT @', 'SELECT $',
                 "SELECT '?', ?", 'SELECT "?", ?', 'SELECT `?`, ?', 'SELECT [?], ?',
                 "SELECT 'O''?雪', ?", 'SELECT "a""?", ?', 'SELECT [a]]?], ?',
                 'SELECT ? -- ?\r\n, ?', 'SELECT ? /* outer /* ? */ ? */, ?',
                 'SELECT /*/**/*/ ?', 'SELECT /* ? */ ?', "SELECT 'unterminated ?",
                 'SELECT "unterminated ?', 'SELECT `unterminated ?', 'SELECT [unterminated ?',
                 'SELECT /* unterminated ?', "SELECT ?, 'unterminated", 'SELECT ?, :later',
                 '雪\0SELECT ?, ?', 'SELECT ?\0, ?', 'SELECT ? -- tail\0?']
    parameters = [[], [number], [values[11], values[9]], [number, null, values[23]]]
    comments = [None, '', ' */ */ ', '\0雪 /* ? --\n */']
    for index, template in enumerate(templates):
        for length, params in enumerate(parameters):
            for failure in [None, 0, 1]:
                item = dict(kind='render', sql=template, params=params,
                            comment=comments[(index+length) % len(comments)])
                if failure is not None:
                    item['literal_failure'] = failure
                result.append(item)
    texts = ['', '\0tail', '雪😀', "quote'\"\\", '\n\r\t', 'x' * 384, '*/ */', 'e\u0301']
    for code in range(12):
        for index, text in enumerate(texts):
            result.append(dict(kind='error', code=code, name=text, reason='reason'+text,
                               identifier='id'+text, version=['-9223372036854775808','0','9223372036854775807'][index % 3],
                               data_type=(code+index) % 10,
                               actual=18446744073709551615 if index >= 4 else index,
                               max=18446744073709551615 if index == 7 else 0 if index == 0 else 63))
    return result


def model_cases(template):
    texts = ['', '\0tail', '雪😀', "quote'\"\\", '\n\r\t', 'x' * 384, '*/ */', 'e\u0301']
    result = []
    for data_type in range(10):
        for index, text in enumerate(texts):
            item = deepcopy(template)
            item.update(text=text, scope=None if index == 0 else index % 4,
                        params=[dict(kind='text', value=text),dict(kind='list', value=[dict(kind='f64', value='NaN')])],
                        untrusted=index % 2 == 0, sql='SELECT ?, ?', comment=None if index == 0 else text,
                        version=['-9223372036854775808','0','9223372036854775807'][index % 3],
                        type=data_type, bytes=18446744073709551615 if index >= 4 else index,
                        outcome=None if index == 0 else index % 3, mode=None if index == 0 else text,
                        omission=None if index <= 1 else text, masked=[True,False,True])
            result.append(item)
    return result
