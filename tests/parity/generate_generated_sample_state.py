#!/usr/bin/env python3
"""Capture original generated sample plan/reference/report helpers."""
import hashlib,itertools,json,sys
from pathlib import Path
from compare_values import observe
ROOT=Path(__file__).resolve().parents[2]
inputs=[]
values=[None,dict(kind='null'),dict(kind='u64',value='18446744073709551615'),dict(kind='i64',value='-1'),dict(kind='i64',value='7'),dict(kind='text',value='12'),dict(kind='bool',value=True),dict(kind='f64',value='2.5')]
for scale,seed,pool,used in itertools.product(['Tiny','Small','Medium'],[0,7,18446744073709551615],[[],[0],[5,7,5,9],[18446744073709551615,0]],[[],[5],[5,7,9],[0,18446744073709551615]]):
    references=[['person',id] for id in pool]+[['Person',77],['platform',1]]
    picks=[dict(entity=entity,salt=salt,used=used) for entity,salt in itertools.product(['person','Person','orders',''],[0,1,3,101])]
    inputs.append(dict(scale=scale,seed=seed,references=references,generated=['orders','person','orders'],skipped=[['orders','Required dependency person is missing in reference pool'],['','long reason '+('x'*256)]],picks=picks,values=values))
producer_dir=ROOT/'tests/generator/parity/checkers_rust'; producer=json.loads((producer_dir/'PROVENANCE.json').read_text())
for file,digest in producer['files'].items():assert hashlib.sha256((producer_dir/file).read_bytes()).hexdigest()==digest,file
lock=json.loads((ROOT/'upstream.lock.json').read_text()); expected=observe(sys.argv[1],inputs)
corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,cases=[dict(input=i,expected=e) for i,e in zip(inputs,expected)])
Path(__file__).with_name('generated_sample_state_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
print(f'Captured {len(inputs)} original sample state observations / {len(inputs)*len(inputs[0]["picks"])} reference choices')
