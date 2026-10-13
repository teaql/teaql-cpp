#!/usr/bin/env python3
"""Compare frozen generated facet requests and actual compact facet results."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]
OPTIONS=['facet_minimal','facet_filter','facet_deleted','facet_slice','facet_comment','facet_raw','facet_membership','aggregate_filter','aggregate_options','aggregate_group','aggregate_nested','with_count','with_stats','with_sum','with_nested_facet','facet_order','outer_filter','outer_deleted','empty','repeat','second']

def cases():
    for minimal,include_all in itertools.product([False,True],[False,True]):
        base=dict(minimal=minimal,include_all=include_all)
        for name in ['_customers','facet with space','', '_sellers']:
            for option in OPTIONS:
                yield dict(base,name=name,**{option:True})
        for features in [['with_count','outer_filter'],['with_stats','outer_filter','aggregate_group'],['with_sum','aggregate_options','facet_comment'],['with_count','facet_membership'],['with_count','aggregate_nested'],['repeat','second','with_count'],['default_options','empty','with_count'],['with_nested_facet','with_count','facet_raw']]:
            yield dict(base,**dict.fromkeys(features,True))
        for feature in [[],['with_count'],['with_stats'],['with_sum'],['with_count','with_stats','with_sum'],['with_count','aggregate_filter','outer_filter'],['with_stats','aggregate_group','outer_filter'],['with_sum','aggregate_options','facet_comment'],['facet_membership','with_count'],['with_nested_facet','with_count'],['repeat','second'],['empty','with_count'],['empty','with_count','outer_deleted'],['facet_filter','facet_minimal'],['facet_slice','facet_order']]:
            for mode,comment in itertools.product(['list','one','page','rows','count','exists'],[False,True]):
                settings=dict.fromkeys(feature,True)
                settings['facet_comment']=comment or settings.get('facet_comment',False)
                yield dict(base,execute=True,mode=mode,**settings)


def error_cases():
    for failure,include_all,mode,comment in itertools.product(['missing_relation','missing_facet_registry','missing_outer_table','missing_facet_table'],[False,True],['list','one','page','rows','count','exists'],[False,True]):
        yield dict(execute=True,include_all=include_all,mode=mode,facet_comment=comment,**{failure:True})


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);parser.add_argument('--diagnostics-dir',type=Path);args=parser.parse_args()
    inputs=list(cases())+list(error_cases());expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    if args.diagnostics_dir:
        args.diagnostics_dir.mkdir(parents=True,exist_ok=True)
        for name,values in [('inputs',inputs),('rust',expected),('cpp',actual)]:
            (args.diagnostics_dir/(name+'.json.gz')).write_bytes(gzip.compress(json.dumps(values).encode()))
    errors=[(i,c,a,b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a,b)]
    for i,c,a,b in errors[:3]:print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if errors:raise SystemExit(f'{len(errors)}/{len(inputs)} facets differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text());producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    fixtures={name:hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() for name in ['dynamic_json_schema.sql','dynamic_json_dataset.sql']}
    corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,fixtures=fixtures,note='Complete unchanged generated crate methods; full SelectQuery Debug, actual SQLite compact facets, outer filtering, mode boundaries and SQL/params/comment/purpose. No output normalization. Existing schema; no bootstrap or cache-execution claim.',cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
    Path(__file__).with_name('generated_facets_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} original/C++ facet observations; {sum(c.get("execute",False) for c in inputs)} actual SQLite executions')
if __name__=='__main__':main()
