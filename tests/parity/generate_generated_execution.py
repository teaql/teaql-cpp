#!/usr/bin/env python3
"""Capture actual generated Rust execution results and ordered SQLite logs."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    filters = {
        'order': [{}, {'code':'Alpha'}, {'code':'absent'}, {'priority':[1,2]},
                  {'customer.name':'Alice'}, {'lines.product.sku':'B'},
                  {'lines.unknown':1}, {'_start':9,'_size':1,'_orderBy':'code'}],
        'person': [{}, {'name':'Alice'}, {'name':'absent'}, {'profile.note':'__is_null__'},
                   {'orders.code':'Alpha'}, {'_start':9,'_size':1,'_orderBy':'name'}],
        'product': [{}, {'sku':'A'}, {'sku':'absent'}, {'price':[2,3]},
                    {'_start':9,'_size':1,'_orderBy':'sku'}],
        'line': [{}, {'quantity':[1,3]}, {'product':{'id':7}}, {'order.customer.name':'Alice'},
                 {'_start':9,'_size':1,'_orderBy':'quantity'}],
    }
    for entity, criteria in filters.items():
        for criterion, minimal, result_type in itertools.product(criteria,[False,True],['native','person']):
            base = dict(entity=entity,filters=[json.dumps(criterion,separators=(',',':'))],minimal=minimal,
                        return_type=result_type,offset=1,limit=1,missing_table=False,optimization='none',max_ids=100)
            for mode in ['count','exists','first','one','list']:
                yield dict(base,mode=mode)
            for offset,limit in [(0,0),(0,1),(1,1),(2,2),(9,1)]:
                yield dict(base,mode='page',offset=offset,limit=limit)
    for entity, mode, minimal, result_type in itertools.product(filters,['count','exists','first','one','page','list'],[False,True],['native','person']):
        yield dict(entity=entity,filters=['{}'],mode=mode,minimal=minimal,return_type=result_type,
                   offset=1,limit=1,missing_table=True,optimization='none',max_ids=100)
    for entity, mode, optimization, max_ids, offset, limit in itertools.product(filters,['count','exists','page','list'],['id_set','continuous'],[0,100],[0,1,9],[0,1]):
        yield dict(entity=entity,filters=['{}'],mode=mode,minimal=False,return_type='native',
                   offset=offset,limit=limit,missing_table=False,optimization=optimization,max_ids=max_ids)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True)
    args=parser.parse_args();inputs=list(cases())
    expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    errors=[(i,c,a,b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    for i,c,a,b in errors[:3]:print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} generated execution cases differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,
                fixtures={name:hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() for name in ['dynamic_json_schema.sql','dynamic_json_dataset.sql']},
                note='Unmodified complete generated crate; actual count/exists/first/one/page/list, result-model adaptation, missing-table errors, ID-set/continuous hints and exact ordered SQLite SQL/parameters/comment/purpose. Explicit existing database; no bootstrap qualification.',
                cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
    Path(__file__).with_name('generated_execution_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} original/C++ generated execution cases')


if __name__=='__main__':main()
