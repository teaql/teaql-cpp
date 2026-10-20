"""Direct original dynamic row-loader controls, independent of typed queries."""
from itertools import product


def row_ids(*ids):
    return [dict(kind='u64',value=number) for number in ids]


def base(**changes):
    row=dict(ids=row_ids(1,2),existing='none',batch='normal',selection='all',payload='overlap')
    row.update(changes)
    return row


def inputs():
    rows=[]
    shapes=[row_ids(),row_ids(1),row_ids(1,1),row_ids(1,2),row_ids(2,1),row_ids(0,1,2)]
    for ids,existing,batch,selection in product(
        shapes,['none','same','equal','old_revision','other_owner','sparse','mixed_second_bad'],
        ['normal','omitted','unrequested','wrong_owner','wrong_revision','wrong_value',
         'unselected','provider_error','missing_provider','empty_payload'],
        ['all','a','b','empty','wrong_type','unknown']):
        rows.append(base(ids=ids,existing=existing,batch=batch,selection=selection))
    for kind,position,batch,selection in product(
        ['missing','null','negative','text','bool','typed_null','f64','i64_one'],[0,1],
        ['normal','provider_error','missing_provider'],['all','a']):
        ids=row_ids(1,2);ids[position]=dict(kind=kind)
        rows.append(base(ids=ids,batch=batch,selection=selection))
    for ids,existing,payload,selection in product(
        [row_ids(1,2),row_ids(1,1,2),row_ids(1,2,1,2)],['none','same','equal'],
        ['split','overlap','null','empty'],['all','a','b']):
        rows.append(base(ids=ids,existing=existing,payload=payload,selection=selection))
    return rows
