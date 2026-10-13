#!/usr/bin/env python3
"""Compare public graph planning scope creation against actual Rust 5.0.6."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe,same
from generate_entity_graph_plan import node,case,v

ROOT=Path(__file__).resolve().parents[2]


def inputs():
    ids=[None,v('null'),v('typed_null'),v('i64','-7'),v('i64','0'),v('u64','0'),
         v('i64','7'),v('u64','7'),v('u64','18446744073709551615'),
         v('decimal','7.0'),v('text','7'),v('f64','7.0')]
    comments=[None,'',' \t','\u3000','root',' root ','\u200b']
    for operation,identity,comment in itertools.product((2,3),ids,comments):
        values={} if identity is None else {'id':identity}
        yield case(node(values=values,operation=operation,comment=comment),descriptors=[])
    for child_operation,identity,comment in itertools.product((2,3),ids,comments):
        values={} if identity is None else {'id':identity}
        child=node('Child',values,child_operation,comment=comment)
        yield case(node(values={'id':v('u64','1')},operation=1,comment='parent',relations={'kids':[child]}))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True)
    args=parser.parse_args();entries=list(inputs())
    original=observe(args.rust,entries);cpp=observe(args.cpp,entries)
    failures=[(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
    for i,entry,a,b in failures[:3]:print(f'Mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
    if failures:raise SystemExit(f'{len(failures)}/{len(entries)} public graph scope cases differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],
        note='Replay of unchanged historical public planning call driver against actual 5.0.6. Timestamp descriptors/values are excluded, so its current-time rendering branch is never used. Full plan/node/batch/scope and provider query observations; no output normalization for these inputs.',
        cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    Path(__file__).with_name('graph_plan_scopes_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(entries)} public graph planning scope observations')


if __name__=='__main__':main()
