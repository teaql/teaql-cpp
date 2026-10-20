"""Guard take-once, ordered journal, sink-failure and Context teardown cases."""
from itertools import product


def cases():
    statement = ((), ('success',), ('fail',), ('drop',), ('unwind',), ('close',),
                 ('unwind_close',), ('success', 'success'), ('fail', 'fail'),
                 ('success', 'fail'), ('fail', 'success'), ('success', 'unwind'),
                 ('fail', 'unwind'), ('success', 'close'), ('fail', 'close'))
    journal = ((), ('record',), ('record', 'record', 'record'), ('record', 'disarm'),
               ('disarm', 'record'), ('record', 'disarm', 'record'), ('record', 'drop'),
               ('record', 'unwind'), ('record', 'close'), ('record', 'unwind_close'),
               ('drop', 'record'), ('record', 'drop', 'record'),
               ('record', 'record', 'unwind'), ('record', 'disarm', 'close'))
    output = []
    for metadata, observer, panic, compiled, actions in product(*([[False, True]] * 4), statement):
        output.append(dict(mode='statement', metadata=metadata, observer=observer,
                           panic_sink=panic, compiled=compiled, actions=list(actions)))
    for observer, panic, compiled, actions in product(*([[False, True]] * 3), journal):
        output.append(dict(mode='journal', metadata=False, observer=observer,
                           panic_sink=panic, compiled=compiled, actions=list(actions)))
    return output
