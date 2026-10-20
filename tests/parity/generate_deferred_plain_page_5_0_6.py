#!/usr/bin/env python3
"""Compare actual ordinary count-first page execution with gated native queries."""
import argparse,gzip,itertools,json,os,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    return [dict(gate=gate,action=action,offset=offset,limit=limit) for gate,action,offset,limit in itertools.product(('none','count','rows'),('resume','cancel','fail'),(0,1,99),(0,1,2,10))]
def main():
    p=argparse.ArgumentParser();p.add_argument('--cpp',required=True);p.add_argument('--rust',required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True);os.environ['TEAQL_LOG_ENDPOINT']='off'
    cases=inputs();original=observe(a.rust,cases);native=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=o,cpp=c) for i,(x,o,c) in enumerate(zip(cases,original,native)) if not same(o,c)]
    if gaps:
        target=ROOT/'docs/parity/deferred_plain_page_5_0_6_initial_gaps.json'
        if not target.exists():target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+'\n')
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f'{len(gaps)}/{len(cases)} original/native deferred ordinary page differences')
    corpus=dict(baseline=json.load(open(ROOT/'upstream.lock.json'))['commit'],producer=json.load(open(ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json')),
        note='Complete unchanged original domain execute_for_page over a test TeaqlRuntime SPI adapter selecting the gated actual domain executor. Native public EntityQuery over the same generated Order model and gated actual executor. Original/native repository, scalar count, typed decode and SQLite algorithms unchanged. Exact stage order/pending/wake, rows/totals/errors, drop/cancel and retry; no normalization.',cases=[dict(input=x,expected=o) for x,o in zip(cases,original)])
    target=Path(__file__).with_name('deferred_plain_page_5_0_6_cases.json.gz')
    if a.check:
        with gzip.open(target,'rt') as f:assert same(corpus,json.load(f))
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print('Compared',len(cases),'actual original/native deferred ordinary page outcomes')
if __name__=='__main__':main()
