"""Closed shapes for the declared Rust/C++ resource ownership adaptation."""


def resource_shapes(inputs, expected, actual):
    if type(expected) is not list or type(actual) is not list or len(expected) != len(inputs) or len(actual) != len(inputs):
        raise RuntimeError('Resource observations must cover every input')
    for language, rows in [('rust', expected), ('cpp', actual)]:
        for row in rows:
            if type(row) is not dict or set(row) != {'semantic', 'ownership'}:
                raise RuntimeError('Resource observation root shape changed')
            if type(row['semantic']) is not dict or set(row['semantic']) != {'hooks', 'rounds'}:
                raise RuntimeError('Resource semantic shape changed')
            if type(row['semantic']['rounds']) is not list or type(row['ownership']) is not list:
                raise RuntimeError('Resource rounds must be lists')
            for value in row['semantic']['rounds']:
                if type(value) is not dict or set(value) != {'created', 'destroyed_after_close', 'steps'}:
                    raise RuntimeError('Resource semantic round shape changed')
            fields = {'during'} if language == 'rust' else {
                'captured', 'during', 'expired_copy_rejected', 'expired_rejected', 'old_borrow_live_before_close'}
            for value in row['ownership']:
                if type(value) is not dict or set(value) != fields:
                    raise RuntimeError('Declared resource ownership shape changed')
