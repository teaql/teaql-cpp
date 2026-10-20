"""Mutation preparation responsibilities, independent of provider execution."""
from itertools import product


def base(**changes):
    row=dict(entry='insert',entity='Parent',behavior='none',policy='none',bootstrap=False,
             checker='observe',shape='standard',id_kind='missing',version_kind='missing',
             id_failure=False,name=True,expected_version=3,old=True)
    row.update(changes)
    return row


def inputs():
    rows=[]
    for entry,entity,behavior,policy,bootstrap,checker in product(
        ['insert','update'], ['Parent','Child'],
        ['none','observe','retarget','deny'], ['none','observe','retarget','deny'],
        [False,True], ['observe','fix','deny']):
        rows.append(base(entry=entry,entity=entity,behavior=behavior,policy=policy,
                         bootstrap=bootstrap,checker=checker))
    kinds=['missing','null','typed_null','i64_zero','u64_zero','positive','negative',
           'text','empty_text','f64_zero','bool']
    for shape,id_kind,version_kind,id_failure in product(
        ['standard','custom','no_id','no_version','missing'], kinds, kinds, [False,True]):
        rows.append(base(shape=shape,id_kind=id_kind,version_kind=version_kind,id_failure=id_failure))
    for checker,name in product(['none','observe','fix','deny'],[False,True]):
        rows.append(base(checker=checker,name=name))
    for expected_version,old,id_kind in product([None,-1,0,9223372036854775807],[False,True],kinds):
        rows.append(base(entry='update',expected_version=expected_version,old=old,id_kind=id_kind))
    return rows
