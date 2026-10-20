"""Retained graph inputs plus independent shared-identity/projection controls."""
import copy
import itertools

from replay_loaded_row_graph_5_0_6 import inputs as retained_inputs, row, smart, u, v


def inputs():
    result = retained_inputs()
    for many, reverse, order, caches, shape, nested, facets, registered, duplicate_owner in itertools.product(
            [False, True], [False, True], [False, True], range(4),
            ['full', 'identity', 'duplicate'], [False, True], [False, True],
            [False, True], [False, True]):
        local, foreign = ('id', 'owner') if reverse else ('link', 'id')
        parents = []
        for index in range(2):
            owner = 1 if index == 0 or duplicate_owner else 2
            identity = index == 1 and shape == 'identity'
            fields = ({foreign: u(10)} if identity else
                      dict(id=u(10), owner=u(owner), label=v('text', 'projection ' + str(index))))
            child_loaded = {}
            if nested and not identity:
                leaves = [row(dict(id=u(100), owner=u(10), label=v('text', 'leaf ' + str(index))))] if index == 0 else []
                fields['leaves'] = v('list', [v('object', leaf['fields']) for leaf in leaves])
                child_loaded['leaves'] = smart(leaves)
            children = [row(fields, child_loaded)]
            if index == 1 and shape == 'duplicate':
                other = copy.deepcopy(children[0])
                other['fields']['label'] = v('text', 'duplicate selected projection')
                children.append(other)
            payload = v('list', [v('object', child['fields']) for child in children])
            cached = {}
            if caches & (1 << index):
                choices = {'choices': smart([row(dict(id=u(90), label=v('text', 'facet ' + str(index))))])} if facets else {}
                cached['children'] = smart(copy.deepcopy(children), choices)
                # Cached rows, rather than this conflicting raw payload, are authoritative.
                payload = v('object', dict(id=u(10), owner=u(owner), label=v('text', 'raw fallback')))
            parents.append(row(dict(id=u(owner), children=payload), cached))
        if order:
            parents.reverse()
        result.append(dict(many=many, local=local, foreign=foreign, registered=registered,
                           rows=parents, control=dict(protocol=True, order=order, caches=caches,
                               shape=shape, nested=nested, facets=facets, duplicate_owner=duplicate_owner)))
    assert len(result) == 3568
    return result
