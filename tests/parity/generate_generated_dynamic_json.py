#!/usr/bin/env python3
"""Compare actual frozen generated JSON translation and SQLite typed reads."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
from compare_generated_dynamic_json import cpp_observation

ROOT=Path(__file__).resolve().parents[2]


def case(entity, sources, execute=False, minimal=False, pre_slice=False, unlimited=False):
    return dict(entity=entity,filters=sources,execute=execute,minimal=minimal,pre_slice=pre_slice,unlimited=unlimited)


def cases():
    fields={'order':['code','amount','priority','customer','customer_id','customer.name','seller.name',
        'customer.profile.note','customer.profile.unknown','lines.quantity','lines.product.sku',
        'lines.order.customer.email','lines.unknown','customer.unknown','customer..name','unknown.name','missing','_ignored'],
        'person':['name','email','profile.note','profile.unknown','orders.code','orders.lines.product.sku','platform.name'],
        'product':['sku','price','line_list.quantity','missing'], 'line':['quantity','product','product_id','order.customer.name']}
    values=[None,False,0,1,2.5,'Alpha',' Alice ','__is_null__','__IS_NOT_NULL__',
            [],[1],[1,3],['Alpha','Beta'],[{'id':2},{'id':3}],{'id':2},{'value':2}]
    for entity,names in fields.items():
        for name,value,minimal in itertools.product(names,values,[False,True]):
            yield case(entity,[json.dumps({name:value},separators=(',',':'))],minimal=minimal)
    controls=[{}, {'_start':2},{'_size':1},{'_pageSize':2},{'_page':2},{'_page':0},
        {'_page':2,'_size':1},{'_page':3,'_size':1,'_pageSize':2,'_start':9},
        {'_page':18446744073709551615,'_pageSize':2},{'_page':2,'_pageSize':0},
        {'_start':-1,'_size':1.0,'_page':'2'},
        {'_orderBy':'code'},{'_orderBy':{'field':'priority','useAsc':True}},
        {'_orderBy':[{'field':'code'},False,{'field':'priority','useAsc':True},{'field':'customer.name'}]},
        {'_orderBy':{'field':'customer'}},{'_orderBy':{'field':'missing'}},
        {'_orderBy':{'field':7,'useAsc':True}},{'_orderBy':{'field':'priority','useAsc':'true'}}]
    for control,pre_slice,unlimited in itertools.product(controls,[False,True],[False,True]):
        yield case('order',[json.dumps(control,separators=(',',':'))],pre_slice=pre_slice,unlimited=unlimited)
    for source in ['null','0','false','[]','""','{','{"code":','not json','{"code":"Alpha","code":"Beta"}']:
        yield case('order',[source])
    for entity,filters in {'order':[{}, {'code':'Alpha'},{'priority':[1,2]},{'customer.name':'Alice'},
            {'lines.product.sku':'B'}, {'lines.unknown':1}, {'customer.profile.note':'__is_null__'},
            {'_orderBy':{'field':'priority','useAsc':True},'_size':1}, {'_page':2,'_pageSize':1,'_orderBy':'code'}],
        'person':[{}, {'profile.note':'__is_null__'},{'orders.lines.product.sku':'B'}, {'name':['Alice','Bob']}],
        'line':[{}, {'order.customer.name':'Alice'},{'product':{'id':7}}, {'quantity':[1,3]}],
        'product':[{}, {'sku':['A','B']},{'price':[2,3]}]}.items():
        for value,minimal in itertools.product(filters,[False,True]):
            yield case(entity,[json.dumps(value,separators=(',',':'))],execute=True,minimal=minimal)
    for sources in [['{"code":"Alpha"}','{"code":"Beta"}'],['{"_size":1}','{"_page":2}'],
                    ['{"customer.unknown":1}','{"lines.unknown":1}'],['{"code":"Alpha"}','{']]:
        yield case('order',sources)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);args=parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    inputs=list(cases());expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    errors=[(i,c,a,b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(cpp_observation(a),b)]
    for i,c,a,b in errors[:3]:print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} generated dynamic JSON cases differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,
        adaptation='C++ descriptors store raw SQL identifiers; only the frozen Line order column debug spelling is adapted from quoted to raw. Original observations are retained verbatim.',
        fixtures={name:__import__('hashlib').sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() for name in ['dynamic_json_schema.sql','dynamic_json_dataset.sql']},
        note='Complete unchanged frozen generated crate. Public filter_with_json, AST/debug/pagination/panic and actual SQLite execute_for_list IDs/versions/errors on the same existing foreign-key-enabled SQL database fixture. Generated ensure_schema is qualified separately as a recorded source failure.',cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
    Path(__file__).with_name('generated_dynamic_json_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    failure=observe(args.rust,[{'kind':'schema'}])[0]
    record=dict(baseline=lock['commit'],producer=producer['generator_commit'],
        fixture=producer['model'],model_sha256=producer['model_sha256'],input={'kind':'schema'},expected=failure,
        note='Unmodified frozen generated service_runtime_from_pool followed by public ensure_schema on a new in-memory database. This failure is preserved separately; paired query execution uses an explicit existing database fixture and makes no successful bootstrap claim.')
    (ROOT/'docs/parity/dynamic_json_schema_source_failure_5_0_6.json').write_text(json.dumps(record,indent=2)+'\n')
    print(f'Compared {len(inputs)} actual original/C++ generated JSON cases; source schema failure recorded')


if __name__=='__main__':main()
