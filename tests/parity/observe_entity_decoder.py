#!/usr/bin/env python3
"""Observe public decoder and RuntimeModule APIs at the locked baseline."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe

def v(kind, value=None):
    return dict(kind=kind, **({} if value is None else {'value':value}))
def row(identifier=None,label='one',fail=False):
    return dict(id=identifier or v('u64','1'),label=v('text',label),fail=v('bool',fail))
def cases():
    cases=[]
    def add(label,steps,mode='decode'):cases.append(dict(label=label,input=dict(mode=mode,steps=steps)))
    def decode(mode,rows,**kw):return {'op':mode,**({'row':rows[0]} if mode=='compact' else {'rows':rows}),**({'relation':'rows' if mode=='list' else 'optional'} if mode in ['list','option'] else {}),**kw}
    for mode in ['compact','batch','list','option']:
        add('unregistered '+mode,[decode(mode,[row()])])
        add('unknown key '+mode,[dict(op='register'),decode(mode,[row()],entity='item')])
        for identifier in [v('null'),v('u64','1'),v('i64','1'),v('i64','-1'),v('text','1'),v('text','bad'),v('bool',True),v('f64','1.0'),v('decimal','1'),v('u64','18446744073709551615')]:
            for fail in [False,True]:
                add(f'{mode} identity {identifier} fail {fail}',[dict(op='register'),decode(mode,[row(identifier,fail=fail)])])
        if mode!='compact':
            for ids in [[],[row(),row(v('u64','2'),'two')],[row(),row(label='replace')],[row(),row(fail=True),row(v('u64','3'))],[row(),row(v('text','bad'))],[row(fail=True),row()]]:
                add(mode+' rows '+str(ids),[dict(op='register'),decode(mode,ids)])
            add(mode+' failed replacement',[dict(op='register'),decode(mode,[row(label='old')]),decode(mode,[row(label='new'),row(fail=True)])])
        add(mode+' registration overwrite',[dict(op='register'),decode(mode,[row()]),dict(op='register',kind=1),decode(mode,[row(label='other type')])])
    add('ledger grouping and graph views',[dict(op='register'),decode('batch',[row(),row(v('u64','2'),'second')]),decode('compact',[row(v('u64','3'),'third')]),decode('list',[row(),row(v('u64','2'))]),decode('option',[row(),row(fail=True)])])
    add('separate compact calls',[dict(op='register'),decode('compact',[row()]),decode('compact',[row(v('u64','2'))])])
    add('copy registry preserves decoder',[dict(op='register'),dict(op='copy',rows=[row()]),decode('compact',[row(v('u64','2'))])])
    add('empty option distinct from missing',[dict(op='register'),decode('option',[],relation='empty')])
    add('list option replacement shares one relation slot',[dict(op='register'),decode('list',[row()]),decode('option',[row()],relation='rows')])
    actions=[dict(op='entity',kind=0),dict(op='entity',kind=1),dict(op='entity',kind=2),dict(op='entity_behavior',value='first'),dict(op='entity_behavior',kind=2,value='alias behavior'),dict(op='behavior',name='Item',value='custom'),dict(op='behavior',name='OnlyBehavior',value='orphan'),dict(op='descriptor',name='Item')]
    for i,(a,b) in enumerate(itertools.product(actions,repeat=2)):
        add(f'module order {i}',[a,b,dict(op='behavior',name='Item',value='last')],mode='module')
    return cases
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    data=cases();outputs=observe(args.rust,[c['input'] for c in data])
    for c,o in zip(data,outputs):c['expected']=o
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    Path(__file__).with_name('entity_decoder_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,ensure_ascii=True,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} locked entity decoder/module cases')
