#!/usr/bin/env python3
"""Compare complete relationship results and explicitly approved quoted-FK cases."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import observe, same
from compatibility_exceptions_5_0_6 import approved_exception
from quoted_fk_debug_adapter_5_0_6 import normalize_quoted_fk_descriptor
ROOT=Path(__file__).resolve().parents[2]


def cpp_observation(source):
    value=copy.deepcopy(source)
    if isinstance(value.get('query'),str):
        # The one previously documented Line raw/quoted descriptor adaptation.
        value['query']=normalize_quoted_fk_descriptor(value['query'])
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
    exception = approved_exception('quoted_fk')
    indices = [i for i, case in enumerate(corpus['cases'])
               if case['qualification'] == 'pending_source_failure_decision']
    if (indices != exception['exception_case_indices'] or len(indices) != 546
            or len(corpus['cases']) != exception['exact_relationship_cases'] + len(indices)
            or any(case['qualification'] not in ('matched', 'pending_source_failure_decision')
                   for case in corpus['cases'])):
        raise SystemExit('Relationship cases exceed the approved quoted-FK exception')
    inputs=[case['input'] for case in corpus['cases']]
    expected=[case['expected'] for case in corpus['cases']]
    if args.rust and not same(expected,observe(args.rust,inputs)):raise SystemExit('Live original observations changed')
    actual=observe(args.cpp,inputs)
    errors=[];pending=0
    for index,(case,value) in enumerate(zip(corpus['cases'],actual)):
        if case['qualification']=='pending_source_failure_decision':
            pending+=1
            # Preserve the immutable source result and require the approved,
            # complete C++ expectation. This is not an exact Rust match.
            target=case['cpp_observation_at_review']
        else:
            target=cpp_observation(case['expected'])
        if not same(target,value):errors.append((index,case['input'],target,value))
    for index,input,target,value in errors[:3]:print(f'Mismatch {index}: {input}\nExpected evidence: {target}\nActual C++: {value}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} relationship evidence records changed')
    print(f'Generated relationship analysis: {len(inputs)-pending} source comparisons pass; '
          f'{pending} approved quoted-FK expectations pass')
    return 0

if __name__=='__main__':raise SystemExit(main())
