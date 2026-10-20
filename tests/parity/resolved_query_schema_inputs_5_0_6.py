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


def dynamic_guard_inputs():
    """Observe intent/provider guards only, not successful indexed hydration."""
    rows=[dict(row,mode="dynamic_guard") for row in dynamic_inputs()
          if not row["cache"] and not row["capture"]]
    assert len(rows)==72
    return rows


def dynamic_loaded_inputs():
    return [dict(row,mode="dynamic_loaded",payload=payload)
            for row in dynamic_guard_inputs() for payload in ["value","null","empty","error"]]


def assert_dynamic_loaded_contract(cases):
    hydrated=0
    for case in cases:
        entry,result=case['input'],case['expected']
        selected=entry['initial_dynamic']
        for key in ['selection_behavior','selection_policy']:
            if entry[key]=='add':selected=True
            elif entry[key]=='remove':selected=False
        loading=selected and entry['initial_dynamic'] and not entry['empty'] and not entry['failure']
        assert len(result['dynamic_calls'])==(2 if loading else 0)
        assert result['cache_events']==[] and len(result['requests'])==2
        for outcome in result['outcomes']:
            assert 'execution_panic' not in outcome
            if entry['failure']:
                assert outcome['error']=='executor failed'
            elif entry['empty']:
                assert outcome==dict(rows=0,schema=dict(rows=[],hydration=[]))
            elif selected and not entry['initial_dynamic']:
                assert 'dynamic-field loading requires validated query intent' in outcome['error']
            elif loading and entry['payload']=='error':
                assert outcome['error']=='FIXTURE_DYNAMIC_FAILED: Item'
            else:
                assert outcome['rows']==1
                state=outcome['schema']['hydration'][0]
                assert state['has_root'] and state['id_loaded'] and not state['result_loaded']
                assert state['a_loaded']==(loading and entry['payload']!='empty')
                assert not state['b_loaded']
                values=None if not loading else {} if entry['payload']=='empty' else {'a':None if entry['payload']=='null' else 'loaded'}
                assert state['values']==values
                hydrated+=int(loading)
    assert hydrated==30
    return dict(cases=len(cases),hydrated_results=hydrated,provider_calls=sum(len(c['expected']['dynamic_calls']) for c in cases))


def enhanced_inputs():
    rows=[dict(row,mode=mode,payload=payload,rewrite_intent=rewrite)
          for row in dynamic_inputs()
          for mode in ['enhanced','enhanced_aggregates','enhanced_owned']
          for payload in ['value','error'] for rewrite in [False,True]]
    assert len(rows)==3456
    return rows


def assert_enhanced_contract(cases):
    for case in cases:
        entry,result=case['input'],case['expected']
        selected=entry['initial_dynamic']
        for key in ['selection_behavior','selection_policy']:
            if entry[key]=='add':selected=True
            elif entry[key]=='remove':selected=False
        final_scope=entry['mode']=='enhanced'
        comment='rewritten query comment' if final_scope and entry['rewrite_intent'] else entry['comment']
        purpose='rewritten query purpose' if final_scope and entry['rewrite_intent'] else entry['purpose']
        assert len(result['requests'])==(1 if entry['cache'] and not entry['failure'] else 2)
        for request in result['requests']:assert request['comment']==comment
        loading=selected and not entry['empty'] and not entry['failure']
        assert len(result['dynamic_calls'])==(2 if loading else 0)
        for call in result['dynamic_calls']:
            assert call==dict(owner='Item',ids=[1],comment=comment,purpose=purpose)
        if entry['capture']:
            first=result['classifications'][0]
            assert first['selected']==(selected if final_scope else entry['initial_dynamic'])
            assert first['comment']==comment
        for outcome in result['outcomes']:
            assert 'execution_panic' not in outcome
            if entry['failure']:assert outcome['error']=='executor failed'
            elif loading and entry['payload']=='error':assert outcome['error']=='FIXTURE_DYNAMIC_FAILED: Item'
            else:
                assert outcome['rows']==(0 if entry['empty'] else 1)
                if not entry['empty']:
                    state=outcome['schema']['hydration'][0]
                    assert state['has_root'] and state['id_loaded'] and not state['b_loaded']
                    assert state['a_loaded']==loading
                    assert state['values']==({'a':'loaded'} if loading else None)
    counts={key:sum(len(c['expected'][key]) for c in cases) for key in ['requests','dynamic_calls','selection_calls','classifications']}
    assert counts==dict(requests=6048,dynamic_calls=864,selection_calls=13824,classifications=4320)
    return dict(cases=len(cases),**counts)
