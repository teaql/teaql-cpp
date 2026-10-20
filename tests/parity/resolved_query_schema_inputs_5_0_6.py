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
