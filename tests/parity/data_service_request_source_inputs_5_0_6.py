"""Source-owned intent, root selection, command identity and batch boundaries."""


def cases():
    def node(kind, name, identity=None, comment='old frame reason'):
        return dict(kind=kind, name=name, id=identity, comment=comment)

    queries = [[]]
    for kind in range(9):
        for name in ['Root', ' \t\u2003', '', '\0', ' 雪Root \n']:
            queries.append([node(kind, name, '0'), node(2, 'items'),
                            node(6, '', comment='stale comment'),
                            node(7, '', comment='stale purpose')])
    queries += [[node(4, 'Ignored'), node(5, 'Ignored'), node(0, ' \u2003'),
                 node(1, 'First'), node(8, 'Second')],
                [node(6, 'First'), node(0, 'Second')],
                [node(8, 'First'), node(7, 'Second')],
                [node(2, 'Ignored'), node(3, 'Ignored'), node(4, 'Ignored'), node(5, 'Ignored')]]
    result = []
    for trace in queries:
        for flags in range(4):
            result.append(dict(kind='query', entity='Leaf\0雪', trace=trace,
                               comment=' owned comment\0雪 ', purpose=' owned purpose\n ',
                               debug=bool(flags & 1), metadata=bool(flags & 2)))
    values = [dict(kind='null'), dict(kind='typed_null')]
    for kind, inputs in [('u64', ['0', '7', '18446744073709551615']),
                         ('i64', ['-1', '0', '7']),
                         ('text', ['0', '+7', '07', '-1', 'bad']),
                         ('f64', ['0', '7', '1.5', '-1', 'nan', 'inf']),
                         ('decimal', ['0', '7', '1.5']),
                         ('date', ['2026-10-18']), ('timestamp', ['42'])]:
        values += [dict(kind=kind, value=value) for value in inputs]
    values += [dict(kind='bool', value=False), dict(kind='bool', value=True),
               dict(kind='json', value=7), dict(kind='json', value='7'),
               dict(kind='list', value=[dict(kind='u64', value='7')]),
               dict(kind='object', value={'id': dict(kind='u64', value='7')})]
    mutations = [[], [node(3, 'Tail', '0'), node(4, 'Provider'), node(5, 'Sql')],
                 [node(8, '', '0')],
                 [node(8, 'Order', '100'), node(8, 'Payment', '301'), node(2, 'items')],
                 [node(6, 'Comment', '4'), node(7, 'Purpose', '5')],
                 [node(8, 'Root', '0'), node(8, 'Root', '0', 'second untouched reason')]]
    for kind in ['insert', 'update', 'delete', 'recover']:
        for identity in values:
            for trace in mutations:
                result.append(dict(kind=kind, entity='Leaf\0雪', comment=' owned mutation\0雪 ',
                                   id=identity, include_id=True, trace=trace))
    for kind in ['insert', 'batch']:
        for trace in mutations:
            result.append(dict(kind=kind, entity='Leaf', comment=' owned outer reason ',
                               id=dict(kind='null'), include_id=False, trace=trace))
    return result
