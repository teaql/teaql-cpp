#!/usr/bin/env python3
"""Compare real consecutive SQLite pages with deferred cursor-store callbacks."""
import argparse,gzip,itertools,json,os,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    modes=('record','miss','hit','error_get','error_put','wrong_entity','wrong_direction','wrong_size','wrong_offset','expired')
    regular = [dict(mode=mode,gate=gate,action=action,direction=direction,size=size) for mode,gate,action,direction,size in itertools.product(modes,('none','get','put'),('resume','cancel'),('asc','desc'),(0,1,2,4))]
    interleaved = [dict(mode=mode,gate=gate,action=action,direction=direction,size=size,interleave=True) for mode,gate,action,direction,size in itertools.product(modes,('get','put'),('resume','cancel'),('asc','desc'),(1,2))]
    return regular + interleaved
def main():
    p=argparse.ArgumentParser();p.add_argument('--cpp',required=True);p.add_argument('--rust',required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True);os.environ['TEAQL_LOG_ENDPOINT']='off'
    cases=inputs();original=observe(a.rust,cases);native=observe(a.cpp,cases)
    gaps=[dict(index=i,input=x,rust=o,cpp=c) for i,(x,o,c) in enumerate(zip(cases,original,native)) if not same(o,c)]
    if gaps:
        target=ROOT/'docs/parity/deferred_continuous_page_5_0_6_initial_gaps.json'
        if not target.exists():target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+'\n')
        print(json.dumps(gaps[:1],indent=2));raise SystemExit(f'{len(gaps)}/{len(cases)} original/native deferred continuous page differences')
    corpus=dict(baseline=json.load(open(ROOT/'upstream.lock.json'))['commit'],producer=json.load(open(ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json')),
        note='Complete original generated list/continuous-page execution; actual SQLite. Deferred cursor-store SPI, ASC/DESC, valid/missing/invalid/expired/error cursors, three consecutive offsets, resume/drop, exact callbacks/keys/boundaries/page results/plans. Only nondeterministic generated cursor ID and absolute expiry are symmetrically projected to syntax/future validity; no error/key/value normalization.',cases=[dict(input=x,expected=o) for x,o in zip(cases,original)])
    target=Path(__file__).with_name('deferred_continuous_page_5_0_6_cases.json.gz')
    if a.check:
        with gzip.open(target,'rt') as f:assert same(corpus,json.load(f))
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print('Compared',len(cases),'actual original/native deferred continuous page outcomes')
if __name__=='__main__':main()
