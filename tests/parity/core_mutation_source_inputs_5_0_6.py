"""Authored mutation roles, real NUL builders and independent batch boundaries."""
from copy import deepcopy


def cases():
    names = ['', '\0', 'a\0b', '雪😀', 'x' * 384, 'quoted"name', '\n\r\t', 'z']
    payloads = [{'kind': 'null'}, {'kind': 'typed_null'}, {'kind': 'f64', 'value': 'NaN'},
                {'kind': 'object', 'value': {'nested': {'kind': 'f64', 'value': 'NaN'}}},
                {'kind': 'list', 'value': [{'kind': 'f64', 'value': 'NaN'}]},
                {'kind': 'text', 'value': 'a\0雪'}, {'kind': 'json', 'value': {}},
                {'kind': 'decimal', 'value': '1.00'}]
    result = []
    for role in ['mutation', 'generated', 'snapshot']:
        for index, name in enumerate(names):
            left = {name: payloads[index], 'second': {'kind': 'u64', 'value': '18446744073709551615'}}
            for change in range(3):
                right = deepcopy(left)
                if change == 1:
                    right[name + 'x'] = right.pop(name)
                elif change == 2:
                    right[name] = {'kind': 'text', 'value': 'replacement\0'}
                result.append(dict(kind='wrapper', type=role, left=left, right=right))
    for index, name in enumerate(names):
        common = dict(kind='builders', entity=name, id=payloads[index])
        values = [dict(op='value', field=name, value=payloads[index]),
                  dict(op='value', field=name + '\0tail', value={'kind': 'text', 'value': name}),
                  dict(op='value', field=name, value={'kind': 'u64', 'value': '18446744073709551615'}),
                  dict(op='value', field=name, value={'kind': 'typed_null'})]
        result.append(dict(common, type='insert', steps=values))
        result.append(dict(common, type='update', steps=[
            dict(op='version', version='-9223372036854775808'), *values,
            dict(op='version', version='9223372036854775807'), dict(op='version', version='0')]))
        result.append(dict(common, type='delete', steps=[
            dict(op='version', version='-9223372036854775808'), dict(op='hard'), dict(op='hard'),
            dict(op='version', version='9223372036854775807'), dict(op='version', version='0')]))
    for command in ['insert', 'update', 'delete', 'recover', 'batch_insert', 'batch_update']:
        for index, name in enumerate(names):
            trace = dict(trace_kind=index % 9, default=False, entity=name,
                         id='18446744073709551615', comment=name + '\0reason')
            fields = {name: {'kind': 'text', 'value': name + '\0last'}}
            if command.startswith('batch_'):
                left = dict(entity=name, values=[fields], trace=[[], [trace]])
                if command == 'batch_update':
                    left.update(ids=[{'kind': 'u64', 'value': '18446744073709551615'}],
                                versions=[None, '-9223372036854775808', '9223372036854775807'],
                                fields=[name, name, ''], old=[None, {}, fields])
            else:
                left = dict(entity=name, id={'kind': 'text', 'value': name + '\0id'},
                            values=fields, trace=[trace], version='-9223372036854775808',
                            old={}, soft=True)
            for change in range(3):
                right = deepcopy(left)
                if change == 1:
                    right['entity'] += '\0changed'
                elif change == 2:
                    if command == 'insert':
                        right['values'][name] = {'kind': 'text', 'value': 'changed'}
                    elif command == 'update':
                        right['old'] = None
                    elif command == 'delete':
                        right['soft'] = False
                    elif command == 'recover':
                        right['version'] = '9223372036854775807'
                    elif command == 'batch_insert':
                        right['trace'].append([])
                    else:
                        right['fields'].append(name)
                result.append(dict(kind='command', type=command, left=left, right=right))
    assert len(result) == 240
    return result
