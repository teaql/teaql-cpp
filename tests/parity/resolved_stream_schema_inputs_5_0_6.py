"""Retain all stream controls and exercise readonly schemas at cursor boundaries."""
from copy import deepcopy
from itertools import product
import gzip
import json
from pathlib import Path


def inputs():
    old=json.loads(gzip.decompress(Path(__file__).with_name('entity_stream_5_0_6_cases.json.gz').read_bytes()))
    rows=[deepcopy(case['input']) for case in old['cases']]
    assert len(rows)==193
    for schema,result_kind,capture,error_at,consume,empty in product(
        ['none','u64','text'],['u64','text','typed_null','null'],[False,True],
        [-1,0,1],['none','pending','one','all'],[False,True]):
        rows.append(dict(schema_probe=True,schema_kind=schema,result_kind=result_kind,
            capture=capture,error_at=error_at,consume=consume,empty=empty,
            shape='root',size=1,limit=None,hard_limit=3,last_first=False,
            purpose='Inspect delayed readonly stream results'))
    assert len(rows)==769
    return rows
