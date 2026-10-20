"""Privacy propagation and post-provider version boundaries for real mutations."""
from copy import deepcopy
from itertools import product
import gzip
import json
from pathlib import Path


def inputs():
    retained=json.loads(gzip.decompress(Path(__file__).with_name('resolved_mutation_execution_5_0_6_cases.json.gz').read_bytes()))
    rows=[deepcopy(case['input']) for case in retained['cases']]
    template=deepcopy(rows[1502])
    template.update(privacy_probe=True,bound_intent=True,replace_intent=False,counts=[1,1],
                    fail=-1,event_fail=-1,cache=False,log=False)
    template['values']['name']=dict(kind='text',value='visible PRIVATE-OPERAND')
    template['values']['note']=dict(kind='text',value='reference PRIVATE-OPERAND 7')
    template['old']=dict(name=dict(kind='text',value='old PRIVATE-OPERAND'),
                         note=dict(kind='text',value='previous PRIVATE-OPERAND 7'))
    template['lineage']=[dict(kind=0,entity='Order',comment='lineage PRIVATE-OPERAND 7')]
    template['trace']=[dict(kind=0,entity='Order',comment='command PRIVATE-OPERAND 7')]
    for i,batch in enumerate(template['batch']):
        batch['note']=dict(kind='text',value=f'row{i} PRIVATE-OPERAND 7')
    template['traces']=[[dict(kind=0,entity='Order',comment='first PRIVATE-OPERAND 7')],
                        [dict(kind=0,entity='Order',comment='second PRIVATE-OPERAND 8')]]
    shapes=[('entity',entry) for entry in ['insert','update','delete','recover']]
    shapes += [('prepared',entry) for entry in ['insert','update']]
    modes=['none','empty','masked','credential','unknown','id']
    for (layer,entry),privacy,(counts,failure),event_failure,old,version in product(
        shapes,modes,[([0],-1),([1],-1),([1],0)],[-1,0],
        [None,template['old']],[None,-1,0,9223372036854775807]):
        row=deepcopy(template)
        row.update(layer=layer,entry=entry,privacy=privacy,counts=counts,fail=failure,
                   event_fail=event_failure,old=deepcopy(old),version=version)
        rows.append(row)
    for entry,privacy,length,versions in product(
        ['batch_insert','batch_update'],modes,range(3),
        [[],[None],[3],[9223372036854775807],[-9223372036854775808,0],[3,9223372036854775807]]):
        row=deepcopy(template)
        row.update(layer='prepared',entry=entry,privacy=privacy,versions=versions)
        row['batch']=row['batch'][:length]
        rows.append(row)
    for entry,privacy in product(['insert','update','delete','recover'],modes):
        row=deepcopy(template)
        row.update(layer='entity',entry=entry,privacy=privacy,replace_intent=True)
        rows.append(row)
    return rows


def assert_privacy_contract(cases):
    """Check positive controls and publication semantics, beyond oracle equality."""
    fresh=cases[3518:]
    assert len(fresh)==1968
    provider_calls=events=panics=0
    canary_controls={mode:False for mode in ['none','empty','id']}
    for case in fresh:
        row,out=case['input'],case['expected']
        mode=row['privacy']
        expected=None if mode=='none' else mode=='empty'
        for request in out['requests']:
            assert request['privacy'] is expected
            assert request['intent']==('replacement event reason' if row['replace_intent'] else 'verify event execution')
        for event in out['event_privacy']:
            assert event['privacy'] is expected
            rendered=json.dumps(event,ensure_ascii=False)
            if mode in ['masked','credential','unknown']:
                assert 'PRIVATE-OPERAND' not in rendered
            elif 'PRIVATE-OPERAND' in rendered:
                canary_controls[mode]=True
        assert len(out['event_privacy'])==len(out['events'])
        provider_calls += len(out['requests'])
        events += len(out['events'])
        if out['outcome'].get('panic'):
            panics += 1
            assert row['entry'] in ['update','batch_update'] and out['requests']
            assert row['version']==9223372036854775807 or 9223372036854775807 in row['versions']
    assert provider_calls==1968 and events==990 and panics==60
    assert all(canary_controls.values())
    return dict(provider_calls=provider_calls,attempted_events=events,
                post_provider_overflow_panics=panics,unmasked_positive_controls=canary_controls)
