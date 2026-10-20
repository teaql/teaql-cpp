"""Independent callback/intent/metadata controls for actual resolved preparation."""
from itertools import product


def inputs():
    rows=[]
    for behavior,policy,entity,relations,bootstrap,intent in product(
        ['none','observe','rewrite','deny'], ['none','observe','rewrite','deny'],
        ['Parent','Other'], ['none','one','duplicate','missing'], [False,True],
        ['valid','missing','blank','bound']):
        rows.append(dict(behavior=behavior,policy=policy,entity=entity,relations=relations,
                         bootstrap=bootstrap,intent=intent,metadata=True,trace=True))
    for metadata,trace in product([False,True],[False,True]):
        for behavior,policy in [('observe','observe'),('rewrite','observe'),('observe','rewrite')]:
            rows.append(dict(behavior=behavior,policy=policy,entity='Other',relations='duplicate',
                             bootstrap=False,intent='valid',metadata=metadata,trace=trace))
    return rows
