"""Record original public audited-with-executor orchestration and typed restoration."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
from generate_ledger_execution import descriptor, seed, v

def case(**kwargs):
    result=dict(values={'id':v('u64','7'),'version':v('i64','3'),'name':v('text','input')},
                runtime=True,seeds=[seed(fresh=True)],fresh=False,removed=False,loaded=None,dirty=None,old=None,
                reason='reason',checker='none',descriptors=[descriptor(),descriptor('Child')],
                rows=[{'id':v('u64','7'),'version':v('i64','9'),'name':v('text','database')}],read_batches=[],
                counts=[1,1,1],read_fail_at=-1,fail=-1,event_fail=-1,id_fail=-1)
    result.update(kwargs);return result

def inputs():
    cases=[]
    for runtime,fresh,removed,checker,rows,seeds in itertools.product(
            [False,True],[False,True],[False,True],['none','record','fill','error'],
            [[],[{'id':v('u64','7'),'name':v('text','database')}]],
            [[],[seed(fresh=True)],[seed(version=3)],[seed(delete=True,version=3)]]):
        cases.append(case(runtime=runtime,fresh=fresh,removed=removed,checker=checker,rows=rows,seeds=seeds))
    for loaded,dirty,old in itertools.product([None,[],['id','name']],
            [None,[],['name']], [None,{'name':v('text','old')} ]):
        cases.append(case(loaded=loaded,dirty=dirty,old=old,seeds=[seed(version=3)],checker='fill'))
    for values,seeds,checker in itertools.product(
            [{},{'id':v('u64','0'),'name':v('text','zero')},{'id':v('typed_null')},
             {'name':v('null')},{'name':v('text','input'),'_is_new':v('bool',True)}],
            [[],[seed(identity=v('u64','0'),fresh=True)], [seed(identity=v('u64','0'),fresh=True,delete=True)]],
            ['none','fill']):
        cases.append(case(values=values,seeds=seeds,checker=checker))
    for error,event_error,read_error,count in itertools.product([-1,0],[-1,0],[-1,0,1],[0,1,2]):
        cases.append(case(seeds=[seed(version=3)],fail=error,event_fail=event_error,read_fail_at=read_error,counts=[count]))
    for descriptors in [[],[descriptor(identity='')],[descriptor(identity='pk')],[descriptor(required=True)]]:
        cases.append(case(descriptors=descriptors,values={'pk':v('u64','7'),'name':v('text','input')},checker='record'))
    for read_batches in [[[],[{'id':v('u64','7'),'name':v('text','created')}]],
                         [[{'id':v('u64','7')}],[]],
                         [[{'id':v('u64','7')}],[{'name':v('text','decode error')}]]]:
        cases.append(case(seeds=[seed(version=3)],read_batches=read_batches))
    # Changing any local relation key must invalidate the immutable loaded graph.
    for fields in [{'name':v('text','scalar')},{'link':v('u64','8')}]:
        rel=dict(name='child',target='Child',local='link',foreign='id',many=False)
        cases.append(case(descriptors=[descriptor(relations=[rel]),descriptor('Child')],
                          seeds=[seed(values=fields,version=3)]))
    for relation_value in [v('null'),v('typed_null'),v('object',{'id':v('u64','8'),'name':v('text','child')}),
                           v('list',[v('object',{'id':v('u64','8'),'name':v('text','child')})])]:
        rel=dict(name='children',target='Child',local='id',foreign='id',many=True)
        cases.append(case(values={'id':v('u64','7'),'children':relation_value},
                          descriptors=[descriptor(relations=[rel]),descriptor('Child')],checker='record'))
    for reason in [' leading reason ','why','审计原因']:
        cases.append(case(reason=reason,checker='fill'))
    return cases

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--rust',required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    cases=inputs();outputs=observe(args.rust,cases);lock=json.loads((root/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],note='Original public save_audited_ledger_entity_with_executor against unmodified locked runtime; handwritten models and provider/checker/sink/ID SPIs only.',cases=[dict(input=i,expected=o)for i,o in zip(cases,outputs)])
    Path(__file__).with_name('audited_save_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Recorded {len(cases)} audited-save observations')

if __name__=='__main__':main()
