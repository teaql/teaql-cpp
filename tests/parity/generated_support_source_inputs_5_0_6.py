"""Complete query-state changes and independent diagnostic traversal branches."""


def cases():
    sequences = [[], ['count'], ['count', 'count', 'clone'], ['empty'],
                 ['facets', 'count', 'facets', 'clone', 'empty'],
                 ['count', 'facets', 'count', 'clone', 'facets']]
    result = [dict(kind='full', profile=profile, field=field, steps=steps)
              for profile in range(4) for field in range(32) for steps in sequences]
    result += [dict(kind='traversal', shape=shape, steps=steps)
               for shape in range(9) for steps in sequences]
    assert len(result) == 822
    return result
