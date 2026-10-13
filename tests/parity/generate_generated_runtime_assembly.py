#!/usr/bin/env python3
"""Capture actual ordinary module factories and generated schema/root bootstrap."""
import gzip
import hashlib
import itertools
import json
import sys
from pathlib import Path
from compare_values import observe
ROOT=Path(__file__).resolve().parents[2]
inputs=[dict(model=model,mode=mode,rounds=rounds,edit=edit)
        for model,mode,rounds,edit in itertools.product(['checkers','inheritance','relations'],['plain','checkers','behaviors','both'],[1,2,3],[False,True])]
producers={}
for name,fixture in [('checkers','checkers'),('inheritance','inheritance'),('relations','relations')]:
    directory=ROOT/'tests/generator/parity'/(fixture+'_rust')
    producer=json.loads((directory/'PROVENANCE.json').read_text())
    for file,digest in producer['files'].items():
        assert hashlib.sha256((directory/file).read_bytes()).hexdigest()==digest,file
    producers[name]=producer
lock=json.loads((ROOT/'upstream.lock.json').read_text())
expected=observe(sys.argv[1],inputs)
corpus=dict(baseline=lock['commit'],version=lock['version'],producers=producers,
            cases=[dict(input=i,expected=e) for i,e in zip(inputs,expected)])
Path(__file__).with_name('generated_runtime_assembly_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
print(f'Captured {len(inputs)} original runtime assembly observations')
