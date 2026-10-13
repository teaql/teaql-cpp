#!/usr/bin/env python3
"""Replay relationship evidence and report unqualified source-result differences."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]


def cpp_observation(source):
    value=copy.deepcopy(source)
    if isinstance(value.get('query'),str):
        # The one previously documented Line raw/quoted descriptor adaptation.
        value['query']=value['query'].replace('column_name: "\\\"order\\\""','column_name: "order"')
    return value


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp',required=True)
    parser.add_argument('--rust')
    parser.add_argument('--require-complete',action='store_true')
    args=parser.parse_args()
    with gzip.open(Path(__file__).with_name('generated_relation_analysis_5_0_6_cases.json.gz'),'rt') as source:corpus=json.load(source)
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    if corpus['baseline']!=lock['commit']:raise SystemExit('Relationship runtime baseline differs')
    producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    if corpus['producer']!=producer:raise SystemExit('Relationship producer provenance differs')
    for name,digest in corpus['fixtures'].items():
        if hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest()!=digest:raise SystemExit(f'Relationship database fixture changed: {name}')
    if args.require_complete and any(case['qualification']!='matched' for case in corpus['cases']):
        raise SystemExit('Complete relationship qualification requires a decision on 546 quoted-FK source-result differences')
    inputs=[case['input'] for case in corpus['cases']]
    expected=[case['expected'] for case in corpus['cases']]
    if args.rust and not same(expected,observe(args.rust,inputs)):raise SystemExit('Live original observations changed')
    actual=observe(args.cpp,inputs)
    errors=[];pending=0
    for index,(case,value) in enumerate(zip(corpus['cases'],actual)):
        if case['qualification']=='pending_source_failure_decision':
            pending+=1
            # Freeze the review evidence without presenting it as Rust parity.
            target=case['cpp_observation_at_review']
        else:
            target=cpp_observation(case['expected'])
        if not same(target,value):errors.append((index,case['input'],target,value))
    for index,input,target,value in errors[:3]:print(f'Mismatch {index}: {input}\nExpected evidence: {target}\nActual C++: {value}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} relationship evidence records changed')
    print(f'Generated relationship analysis: {len(inputs)-pending} source comparisons pass; {pending} review observations remain UNQUALIFIED pending the source-failure decision')
    if pending and args.require_complete:raise SystemExit('Complete relationship qualification cannot skip the source-failure decision')
    return 0

if __name__=='__main__':raise SystemExit(main())
