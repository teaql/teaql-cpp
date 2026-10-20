"""Keep public query/capture controls, then observe readonly result schema on cache paths."""
from copy import deepcopy
from itertools import product
import gzip
import json
from pathlib import Path


def inputs():
    retained=json.loads(gzip.decompress(Path(__file__).with_name('entity_query_capture_5_0_6_cases.json.gz').read_bytes()))
    rows=[deepcopy(case['input']) for case in retained['cases']]
    for mode,schema,result_kind,capture,(cached,backend,record_cache),empty,failure in product(
        ['list','owned','rows','aggregates'],['none','u64','text'],
        ['u64','text','typed_null','null'],[False,True],
        [(False,False,False),(True,False,False),(True,True,False),(True,True,True)],
        [False,True],[False,True]):
        rows.append(dict(mode=mode,schema_probe=True,schema_kind=schema,result_kind=result_kind,
            capture=capture,select_capture=capture,mutation_capture=not capture,cache=cached,
            backend=backend,record_cache=record_cache,empty=empty,failure=failure,
            continuous=False,aggregate=False,comment='inspect readonly result',
            purpose='verify result schema and cache boundaries',limit=None,calls=2))
    return rows


def typed_inputs():
    rows=[]
    for schema,result_kind,capture,cached,empty,failure in product(
        ['none','u64','text'],['u64','text','typed_null','null'],
        [False,True],[False,True],[False,True],[False,True]):
        rows.append(dict(mode='typed',schema_probe=True,schema_kind=schema,result_kind=result_kind,
            capture=capture,select_capture=capture,mutation_capture=not capture,cache=cached,
            backend=True,record_cache=True,empty=empty,failure=failure,
            continuous=False,aggregate=False,comment='inspect typed readonly result',
            purpose='verify resolved typed decoder',limit=None,calls=2))
    return rows


def dynamic_inputs():
    rows=[]
    for initial,behavior,policy,capture,cached,empty,failure in product(
        [False,True],['keep','add','remove'],['keep','add','remove'],
        [False,True],[False,True],[False,True],[False,True]):
        rows.append(dict(mode='typed',schema_probe=True,dynamic_probe=True,
            initial_dynamic=initial,selection_behavior=behavior,selection_policy=policy,
            schema_kind='u64',result_kind='text',capture=capture,select_capture=capture,
            mutation_capture=not capture,cache=cached,backend=True,record_cache=True,
            empty=empty,failure=failure,continuous=False,aggregate=False,
            comment='inspect prepared dynamic selection',
            purpose='verify optional dynamic fields and callback ordering',limit=None,calls=2))
    assert len(rows)==288
    return rows


def assert_dynamic_contract(cases):
    for case in cases:
        entry,result=case['input'],case['expected']
        assert result['cache_events']==[]
        assert len(result['requests'])==entry['calls']==2
        assert len(result['classifications'])==(2 if entry['initial_dynamic'] and entry['capture'] else 0)
        selected=entry['initial_dynamic']
        events=[]
        for stage,key in [('behavior','selection_behavior'),('policy','selection_policy')]:
            before=selected
            if entry[key]=='add':selected=True
            elif entry[key]=='remove':selected=False
            events.append(dict(stage=stage,before=before,after=selected))
        assert result['selection_calls']==events*2
        for outcome in result['outcomes']:
            if entry['failure']:
                assert 'executor failed' in outcome['error']
            elif selected and not entry['empty']:
                assert 'dynamic fields require a runtime-owned indexed entity carrier' in outcome['error']
            else:
                assert outcome['rows']==(0 if entry['empty'] else 1)
    return dict(cases=len(cases),provider_requests=sum(len(c['expected']['requests']) for c in cases),
                ordered_callbacks=sum(len(c['expected']['selection_calls']) for c in cases),cache_events=0)
