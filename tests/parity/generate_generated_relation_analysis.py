#!/usr/bin/env python3
"""Observe real frozen relation requests, nested metrics and actual SQLite SQL."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
from compare_values import observe, same
from compare_generated_relation_analysis import cpp_observation
ROOT=Path(__file__).resolve().parents[2]
NUMERIC=['sum','avg','min','max','standard_deviation','square_root_of_population_standard_deviation','sample_variance','sample_population_variance']
COMMON=['have','have_no','matching','without','count','count_as','count_with','stats','stats_as','group_details']+NUMERIC+[n+'_as' for n in NUMERIC]
FORWARD=['have_customer','have_no_customer','matching_customer','without_customer','group_customer','group_customer_details','group_customer_from','roll_customer','roll_customer_with']
COLUMNS=['metric','_metric','count_orders','sum_amount_of_orders','avg_amount_of_orders','min_amount_of_orders','max_amount_of_orders','count_lines','sum_quantity_of_lines','avg_quantity_of_lines','min_quantity_of_lines','max_quantity_of_lines','refinements','_outer_count']

def construction_cases():
    for entity,minimal in itertools.product(['person','order'],[False,True]):
        actions=COMMON+(['have_profile','have_no_profile'] if entity=='person' else FORWARD)
        for action,alias in itertools.product(actions,['metric','_metric','with space','']):
            for config in [{},{'child_minimal':True},{'child_deleted':True},{'child_slice':True},{'child_group':True},{'child_options':True}]:
                yield dict(entity=entity,minimal=minimal,actions=[action],alias=alias,cutoff=5,**config)
        for actions in [['count','sum','avg'],['matching','count_with'],['stats_as','stats_as'],['have','have_no']]:
            yield dict(entity=entity,minimal=minimal,actions=actions,alias='metric',cutoff=5,child_options=True)

def execution_cases():
    for entity,minimal in itertools.product(['person','order'],[False,True]):
        actions=COMMON+(['have_profile','have_no_profile'] if entity=='person' else FORWARD)
        for action in actions:
            configs=[{}, {'empty':True}, {'cutoff':1000}, {'child_deleted':True}, {'child_slice':True}, {'child_group':True}, {'child_options':True}, {'child_minimal':True}]
            for config in configs:
                for mode in ['list','rows','one','page']:
                    yield dict(entity=entity,minimal=minimal,actions=[action],alias='_metric',execute=True,columns=COLUMNS,mode=mode,**config)
        for actions in [['count','sum','avg'],['matching','count_with'],['stats_as','stats_as']]:
            yield dict(entity=entity,minimal=minimal,actions=actions,alias='_metric',cutoff=5,execute=True,columns=COLUMNS,mode='list')

def decoding_cases():
    payloads=[None,False,True,0,42,-1,1.5,'scalar',{}, {'id':2,'version':1,'name':'Alice'}, [], [None], [42], ['scalar'], [{}], [{'id':20,'version':1,'quantity':2}], [{'id':20,'version':1},None], {'id':'bad'}]
    for relation in ['customer','lines']:
        yield dict(kind='decode',relation=relation)
        for payload in payloads:
            yield dict(kind='decode',relation=relation,payload=payload)


def known_line_attachment_difference(source, value):
    if len(source.get('rows',[])) != len(value.get('rows',[])):
        return False
    differences=0
    for a,b in zip(source.get('rows',[]),value.get('rows',[])):
        if not isinstance(a,dict) or not isinstance(b,dict):
            return False
        if {k:v for k,v in a.items() if k!='values'} != {k:v for k,v in b.items() if k!='values'}:
            return False
        if len(a['values']) != len(b['values']):
            return False
        for av,bv in zip(a['values'],b['values']):
            if av != bv:
                if av not in ['U64(0)','List([])'] or bv is None:
                    return False
                differences+=1
    return differences > 0


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True);parser.add_argument('--diagnostics-dir',type=Path);args=parser.parse_args()
    inputs=list(construction_cases())+list(execution_cases())+list(decoding_cases());expected=observe(args.rust,inputs);actual=observe(args.cpp,inputs)
    if args.diagnostics_dir:
        args.diagnostics_dir.mkdir(parents=True,exist_ok=True)
        for name,values in [('inputs',inputs),('rust',expected),('cpp',actual)]:
            (args.diagnostics_dir/(name+'.json.gz')).write_bytes(gzip.compress(json.dumps(values).encode()))
    failures=[]
    for index,(input,source,value) in enumerate(zip(inputs,expected,actual)):
        if not same(cpp_observation(source),value):
            # Only the known Line quoted-FK attachment defect is eligible for review.
            keys={key for key in set(source)|set(value) if cpp_observation(source).get(key)!=value.get(key)}
            if (input.get('entity')!='order' or not input.get('execute') or keys!={'rows'} or
                    source.get('sql')!=value.get('sql') or source.get('error') is not None or
                    not known_line_attachment_difference(source,value)):
                print(f'Unclassified mismatch {index}: {input}\nRust: {source}\nC++: {value}')
                raise SystemExit('Unclassified relationship mismatch; no corpus qualification written')
            failures.append(index)
    lock=json.loads((ROOT/'upstream.lock.json').read_text());producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    fixtures={name:hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() for name in ['dynamic_json_schema.sql','dynamic_json_dataset.sql']}
    corpus=dict(baseline=lock['commit'],version=lock['version'],producer=producer,fixtures=fixtures,note='Actual unchanged generated crate methods; complete SelectQuery Debug, typed attached metrics/object details, raw row execution and actual SQLite SQL/params/comment/purpose. Existing FK-enabled schema; bootstrap and facets are not qualified. Original observations are unmodified; the single documented Line descriptor spelling adaptation is separate from pending quoted-FK result differences. Pending C++ observations are review snapshots, not parity acceptance.',cases=[dict(input=c,expected=v,qualification='pending_source_failure_decision' if i in failures else 'matched',**({'cpp_observation_at_review':actual[i]} if i in failures else {})) for i,(c,v) in enumerate(zip(inputs,expected))])
    Path(__file__).with_name('generated_relation_analysis_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Recorded {len(inputs)} original/C++ relation analysis observations; {len(inputs)-len(failures)} source comparisons pass with the declared descriptor adaptation; {len(failures)} result differences remain pending review')
if __name__=='__main__':main()
