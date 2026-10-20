#!/usr/bin/env python3
"""Compare actual constant relationship bootstrap with registered extensions."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for state,module,target,checker,behavior,rounds in itertools.product(
            ('fresh','existing','drift','missing','deleted'),
            ('plain','checker','behavior','combined'),('category','status'),
            ('none','observe','fix','deny'),('none','observe','deny'),(1,2)):
        yield dict(state=state,module=module,target=target,checker=checker,
                   behavior=behavior,rounds=rounds,logging=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',required=True,choices=('ordered','reversed','self_reference_required'))
    parser.add_argument('--cpp',required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record',action='store_true')
    args=parser.parse_args()
    if args.record and not args.rust:parser.error('--record requires the original-source observer')
    os.environ['TEAQL_LOG_ENDPOINT']='off'
    inputs=list(cases())
    path=Path(__file__).with_name(f'constant_topology_extensions_{args.case}_5_0_6_cases.json.gz')
    if args.record:expected=observe(args.rust,inputs)
    else:
        corpus=json.loads(gzip.decompress(path.read_bytes()))
        assert corpus['baseline']==json.loads((ROOT/'upstream.lock.json').read_text())['commit']
        assert same(inputs,[case['input'] for case in corpus['cases']])
        expected=[case['expected'] for case in corpus['cases']]
        if args.rust and not same(expected,observe(args.rust,inputs)):
            raise RuntimeError('Topology extension corpus differs from fresh original')
    actual=observe(args.cpp,inputs)
    gaps=[dict(index=i,input=value,rust=source,cpp=native)
          for i,(value,source,native) in enumerate(zip(inputs,expected,actual)) if not same(source,native)]
    if gaps:
        out=ROOT/'docs/parity'/f'constant_topology_extensions_{args.case}_5_0_6_initial_gaps.json.gz'
        if not out.exists():out.write_bytes(gzip.compress(json.dumps(gaps,indent=2).encode(),mtime=0))
        print(json.dumps(gaps[0],indent=2));raise SystemExit(f'{len(gaps)} topology extension differences; see {out}')
    if args.record:
        path.write_bytes(gzip.compress(json.dumps(dict(
            baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
            case=args.case,scope='Complete unchanged generated domains/runtime/SQLite algorithms. '
            'Public module/Checker/Behavior registration; actual bootstrap rows, relationship Q/E, '
            'ordered callbacks, full errors and logs, and database after actual Context close. No normalization.',
            cases=[dict(input=value,expected=source) for value,source in zip(inputs,expected)]),
            separators=(',',':')).encode(),mtime=0))
    print(f'{args.case}: {len(inputs)}/{len(inputs)} exact topology/extension outcomes')


if __name__=='__main__':main()
