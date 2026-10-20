"""Comment folding and independently instrumented observation controls."""
import itertools


def inputs():
    kinds = ['Operation', 'Request', 'Relation', 'Entity', 'Provider', 'Sql',
             'Comment', 'Purpose', 'AuditReason']
    nodes = [dict(kind=kind, entity='Item', id=identity, comment='reason\n😀')
             for kind, identity in itertools.product(kinds, [None, '0', str(2**64-1)])]
    traces = [[]] + [[node] for node in nodes]
    traces += [[dict(kind='Request', entity='', id=None, comment=''),
                dict(kind='AuditReason', entity='子\x00', id='7', comment=' -> []'),
                dict(kind='Entity', entity='Item', id='0', comment=' ')] , nodes * 4]
    result = [dict(mode='comment', user=user, comment=comment, trace=trace)
              for user, comment, trace in itertools.product(
                  [None, '', ' ', 'actor', '用户', '\x00[\n]'],
                  [None, '', ' ', 'reason', '😀\n', '\x00'], traces)]
    result += [dict(mode='observe', noop=noop, failure=failure, name_panic=name_panic,
                    pending=pending, polls=polls)
               for noop, failure, name_panic, pending, polls in itertools.product(
                   [False, True], [False, True], [False, True], [0, 2], [0, 1, 4])]
    return result
