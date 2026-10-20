#!/usr/bin/env python3
"""Compare actual generated pages through deferred custom ID-store callbacks."""
import argparse,gzip,itertools,json,os,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]

def inputs():
    gates={'miss':('none','get1','get2','put'),'error_put':('none','get1','get2','put'),
           'error_get':('none','get1'),'hit':('none','get1'),'empty':('none','get1')}
    for mode,positions in gates.items():
        for gate,action,offset,limit in itertools.product(positions,('resume','cancel'),(0,1,4),(0,1,2)):
            yield dict(mode=mode,gate=gate,action=action,offset=offset,limit=limit)

def main():
    p=argparse.ArgumentParser();p.add_argument('--cpp',required=True);p.add_argument('--rust',required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    os.environ['TEAQL_LOG_ENDPOINT']='off'
    cases=list(inputs());original=observe(a.rust,cases);native=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=o,cpp=c) for i,(x,o,c) in enumerate(zip(cases,original,native)) if not same(o,c)]
    if gaps:
        target=ROOT/'docs/parity/deferred_id_set_page_5_0_6_initial_gaps.json'
        if not target.exists():target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+'\n')
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f'{len(gaps)}/{len(cases)} original/native deferred page differences')
    corpus=dict(baseline=json.load(open(ROOT/'upstream.lock.json'))['commit'],producer=json.load(open(ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json')),
        note='Complete original domain and public execute_for_page; actual SQLite ID/read/count operations. Controlled custom ID-store SPI suspends get1/get2/put, then resume/drop and retry. Exact callback keys/order/data, page rows/totals, wake/pending/cancel outcomes. Original Context is immutable while borrowed; C++ replacement/destruction adaptation remains separately qualified. No normalization.',cases=[dict(input=x,expected=o) for x,o in zip(cases,original)])
    target=Path(__file__).with_name('deferred_id_set_page_5_0_6_cases.json.gz')
    if a.check:
        with gzip.open(target,'rt') as f:assert same(corpus,json.load(f))
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print('Compared',len(cases),'actual original/native deferred ID-set page outcomes')
if __name__=='__main__':main()
