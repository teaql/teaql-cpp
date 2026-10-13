#!/usr/bin/env python3
"""Compare actual generated cache SPI keys/events, rows and SQL emission."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]

def cases():
    configurations=[{},dict(age=0),dict(age=1000000),dict(child_age=0),dict(child_age=1000000),dict(propagate=0),dict(propagate=1000000),dict(age=0,propagate=0),dict(age=1000000,propagate=1000000),dict(age=0,propagate=0,child_age=1000000)]
    for kind,config in itertools.product(['count','sum'],configurations):
        for alteration in [{},dict(comment='changed comment'),dict(purpose='changed purpose'),dict(filter=True),dict(child_age=0),dict(clear=True),dict(update=True),dict(save=True),dict(empty=True)]:
            yield dict(kind=kind,steps=[dict(config),dict(config,**{})|alteration,dict(config)])
        yield dict(kind=kind,no_backend=True,steps=[dict(config),dict(config,update=True)])
    # Wide timing separation; zero TTL never expires. Only four short real-clock runs.
    for kind,age in itertools.product(['count','sum'],[0,1]):
        yield dict(kind=kind,steps=[dict(child_age=age),dict(child_age=age,delay=30,update=True)])

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);parser.add_argument('--diagnostics-dir',type=Path);args=parser.parse_args()
    inputs=list(cases());expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    if args.diagnostics_dir:
        args.diagnostics_dir.mkdir(parents=True,exist_ok=True)
        for name,values in [('inputs',inputs),('rust',expected),('cpp',actual)]:
            (args.diagnostics_dir/(name+'.json.gz')).write_bytes(gzip.compress(json.dumps(values).encode()))
    errors=[(i,c,a,b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    for i,c,a,b in errors[:3]:print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} cache observations differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    fixtures={name:hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() for name in ['dynamic_json_schema.sql','dynamic_json_dataset.sql']}
    corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,fixtures=fixtures,note='Actual unchanged generated request methods and SQLite. Recording wrapper delegates unmodified InMemoryAggregationCache; records exact keys, requested age, hit/put events, values and cumulative SQL logs. No normalization. Source runtime-scope telemetry and governed invalidation remain separately qualified.',cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
    Path(__file__).with_name('generated_aggregation_cache_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} original/C++ cache scenarios and {sum(len(c["steps"]) for c in inputs)} real generated SQLite executions')
if __name__=='__main__':main()
