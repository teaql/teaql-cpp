"""Finite direct SmartList boundary cases; metadata is observed, not reconstructed."""
from copy import deepcopy
from itertools import product


def cases():
    def v(kind, value):
        return {'kind':kind,'value':value}

    def row(identity='1', fail=False):
        result = {'id':v('u64',identity),'label':v('text','registry projection')}
        if fail:
            result['fail']=v('bool',True)
        return result

    def empty():
        return dict(rows=[], total=None, loaded=True, aggregations={}, summary={}, facets={})

    def metadata(profile):
        output=empty()
        if profile:
            output['aggregations']={'count':v('u64','18446744073709551615'), 'zero':v('i64','0')}
            output['summary']={'label':v('text','summary with Unicode \u4e2d\u6587'), 'null':{'kind':'null'}}
        if profile==2:
            leaf=empty();leaf.update(rows=[row('9')], total=0, loaded=False)
            leaf['aggregations']={'sum':v('i64','-7')}
            output['facets']={'detail':leaf, 'empty':empty()}
            nested=deepcopy(leaf);nested['facets']={'nested':empty()}
            output['facets']['nested']=nested
        return output

    patterns=[[],[row()],[row(),row()], [row(fail=True)],
              [row(),row('2',True),row('3')],[{'label':v('text','without id')}]]
    return [dict(mode=mode, registration=registration, seed=seed,
                 list=dict(metadata(profile), rows=deepcopy(rows), loaded=loaded,total=total))
            for mode,profile,loaded,total,rows,registration,seed in product(
                ('smart','vec'),range(3),(False,True),(None,0,2**64-1),patterns,range(3),(False,True))]
