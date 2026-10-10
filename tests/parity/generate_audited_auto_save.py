"""Record direct ordinary/automatic audited entries against the locked source."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
from generate_audited_save import case as inner_case, inputs as inner_inputs
from generate_ledger_execution import seed,v

def case(**kwargs):
    result=inner_case();result.update(mode='automatic',registered=True,ambient_rows=[],
        begin_fail=False,commit_fail=False,rollback_fail=False)
    result.update(kwargs);return result

def inputs():
    cases=[]
    for original in inner_inputs():
        for mode in ['plain','automatic']:
            item=case(**original);item['mode']=mode;cases.append(item)
    for mode,runtime,checker,registered,fresh,removed in itertools.product(
            ['plain','automatic'],[False,True],['none','fill','error'],[False,True],[False,True],[False,True]):
        cases.append(case(mode=mode,runtime=runtime,checker=checker,registered=registered,
                          fresh=fresh,seeds=[seed(fresh=True,delete=removed)]))
    for mode,begin_fail,commit_fail,rollback_fail,error,event_error,read_error in itertools.product(
            ['plain','automatic'],[False,True],[False,True],[False,True],[-1,0],[-1,0],[-1,0]):
        cases.append(case(mode=mode,begin_fail=begin_fail,commit_fail=commit_fail,
                          rollback_fail=rollback_fail,fail=error,event_fail=event_error,read_fail_at=read_error))
    for mode,seeds,rows in itertools.product(['plain','automatic'],
            [[],[seed(version=3)]],[[],[{'id':v('u64','7'),'name':v('text','decode error')}]]):
        cases.append(case(mode=mode,seeds=seeds,rows=rows,ambient_rows=[{'id':v('u64','7')}],checker='record'))
    return cases

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--rust',required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2];subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    cases=inputs();outputs=observe(args.rust,cases);lock=json.loads((root/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],note='Direct unmodified public AuditedSaveExt and save_audited_ledger_entity calls with handwritten models/SPIs. Only the compiler-specific missing-saver type suffix is normalized; message prefix, failure ordering and all other observations remain exact.',cases=[dict(input=i,expected=o)for i,o in zip(cases,outputs)])
    Path(__file__).with_name('audited_auto_save_cases.json').write_text(json.dumps(corpus,indent=2)+'\n');print(f'Recorded {len(cases)} automatic audited observations')

if __name__=='__main__':main()
