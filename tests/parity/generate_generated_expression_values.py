#!/usr/bin/env python3
"""Capture original E Value/Null/fallback behavior without result normalization."""
import hashlib,itertools,json,sys
from pathlib import Path
from compare_values import observe
ROOT=Path(__file__).resolve().parents[2]
notes=[None,'','plain text','  padded text  ','Unicode 数据 and a quoted "value"', 'x'*4096]
inputs=[]
for id,note,change,next_value in itertools.product([0,7],notes,[False,True],[None,'next text']):
    inputs.append(dict(mode='note',id=id,note=note,change=change,next=next_value))
for id,note,empty in itertools.product([0,7],notes,[False,True]):
    inputs.append(dict(mode='profile',id=id,note=note,empty=empty))
for id,count,index,loaded in itertools.product([0,7],[0,1,2],[0,1,7],[False,True]):
    inputs.append(dict(mode='orders',id=id,count=count,index=index,loaded_flag=loaded))
producer_dir=ROOT/'tests/generator/parity/dynamic_json_rust'
producer=json.loads((producer_dir/'PROVENANCE.json').read_text())
for name,digest in producer['files'].items():
    assert hashlib.sha256((producer_dir/name).read_bytes()).hexdigest()==digest,name
expected=observe(sys.argv[1],inputs)
lock=json.loads((ROOT/'upstream.lock.json').read_text())
corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,
            cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
Path(__file__).with_name('generated_expression_values_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2,ensure_ascii=False)+'\n')
print(f'Captured {len(inputs)} original generated E value observations')
