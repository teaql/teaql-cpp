#!/usr/bin/env python3
"""Check actual denied I/O, raw trust and precise privacy regressions in paired evidence."""
import gzip
import json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests/parity'))
from compare_values import same


def main():
    record=json.loads(gzip.decompress((ROOT/'docs/parity/composed_general_repaired_5_0_6.json.gz').read_bytes()))
    assert not record['failures'] and len(record['inputs'])==4864
    assert same(record['original'],record['cpp'])
    blank=denied=protected=callbacks=logs=0
    for case,result in zip(record['inputs'],record['cpp']):
        assert result['released'],case
        callbacks+=len(result['policy']);logs+=len(result['logs'])
        if not case['logging']:assert result['logs']==[],case
        if case['intent']!='valid':
            blank+=1
            assert result['panic']==('query comment must not be empty' if case['intent']=='blank_comment' else 'query purpose must not be empty')
            assert result['policy']==[] and result['logs']==[] and result['selects']==0 and result['rows']==[],case
        elif case['policy']=='deny' or (case['policy']=='reject_raw' and any(call['raw'] for call in result['policy'])):
            denied+=1
            assert result['error']=='graph write error: request policy error: COMPOSED_QUERY_DENIED',case
            assert result['selects']==0 and result['logs']==[] and result['rows']==[],case
        if case['intent']=='valid' and case['logging'] and case['policy'] in ['allow','restrict'] and case['shape']=='raw_outer' and (
                case['mode']=='stream' or (case['mode']=='page' and case['optimization']=='continuous')):
            protected+=1
            assert len(result['logs'])==1,case
            for key in ['comment','purpose']:
                assert 'PRIVATE-SCHOOL-CANARY' not in result['logs'][0][key] and '[REDACTED]' in result['logs'][0][key],case
    assert (blank,denied,protected,callbacks,logs)==(768,1536,16,4664,1528)
    print(f'Verified {blank} pre-Policy intent failures, {denied} pre-I/O denials, {protected} precise privacy repairs, {callbacks} full Policy snapshots and {logs} safe SQL records')


if __name__=='__main__':main()
