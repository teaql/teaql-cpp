#!/usr/bin/env python3
"""Compare complete original generated query descriptions and failure messages."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same

ROOT=Path(__file__).resolve().parents[2]

def cases():
    operations=['filter','live','or_live','append','with_deleted','deleted_only','select','project','raw','unsafe_raw','unselect','negative','not_start','not_end','sound','gbk','raw_sql','unsafe_sql','raw_filter','unsafe_filter','property_as','unsafe_property_as','stream_default','threshold']
    for entity,minimal in itertools.product(['order','person'],[False,True]):
        base=dict(entity=entity,minimal=minimal)
        for operation in operations:
            for value in ['Alpha','',"'private text'",'value + 1']:
                yield dict(base,ops=[dict(op=operation,value=value,number=0)])
        for kind in ['eq','ne','gt','gte','lt','lte','contain','not_contain','begin','not_begin','end','not_end','between','in','null','sound']:
            yield dict(base,ops=[dict(op='property',value=kind)])
        for op,number,size in itertools.product(['page_offset','offset','offset_size','page_number','page'],[0,1,2,18446744073709551615],[0,1,3]):
            yield dict(base,ops=[dict(op=op,number=number,size=size)])
        for op,number in itertools.product(['offset_only','page_number_default','page_default','top'],[0,1,9,18446744073709551615]):
            yield dict(base,ops=[dict(op=op,number=number,size=number)])
        for names in [['filter','with_deleted'],['or_live','with_deleted'],['with_deleted','deleted_only','deleted_only'],
                      ['raw','raw','select','unselect'],['select','project','unselect','select'],
                      ['raw_filter','unsafe_filter','raw_sql','property_as'],
                      ['page_offset','offset_only','top','unlimited','page_default']]:
            yield dict(base,ops=[dict(op=op,value="'safe value'",number=2,size=3) for op in names])

def execution_cases():
    for entity,minimal in itertools.product(['order','person'],[False,True]):
        base=dict(entity=entity,minimal=minimal,execute=True)
        for names in [[],['with_deleted'],['deleted_only'],['deleted_only','with_deleted'],['with_deleted','deleted_only'],
                      ['negative'],['not_start'],['not_end'],['sound'],['raw'],['unsafe_raw'],['raw','unselect'],
                      ['property_as'],['unsafe_property_as'],['page_default'],['page_number'],['offset_only'],
                      ['top'],['page_offset'],['gbk'],['raw_filter'],['unsafe_filter']]:
            yield dict(base,ops=[dict(op=op,value="'fixed'" if op in ['raw','unsafe_raw'] else '41' if 'property_as' in op else '1=1' if 'filter' in op else 'Alpha',number=2,size=1) for op in names])
        for kind in ['eq','ne','gt','lt','contain','not_contain']:
            yield dict(base,ops=[dict(op='property',value=kind)])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);args=parser.parse_args()
    inputs=list(cases())+list(execution_cases());expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    errors=[(i,c,a,b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    for i,c,a,b in errors[:3]:print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} general query cases differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,note='Complete unmodified generated crate; actual request methods and full SelectQuery Debug/panic messages. No output normalization. Full construction and actual SQLite execute_for_rows on the fixed existing FK-enabled database; no bootstrap claim.',cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
    Path(__file__).with_name('general_query_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} original/C++ general query cases')

if __name__=='__main__':main()
