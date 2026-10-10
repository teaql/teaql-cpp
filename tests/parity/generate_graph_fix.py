"""Record original evidence/private validation/scope/audited-save observations."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def value(kind, data=None):
    out=dict(kind=kind)
    if data is not None: out['value']=data
    return out


def inputs():
    cases=[]
    fields=['', ' ', '\u2003\u3000', '\u200b', 'Item', ' Item ', '😀', 'a\x00b']
    labels=['', ' ', '\u2003', 'clock', 'context.root', ' Authorization ', 'COOKIE', 'TOKEN=value',
            'xCookieSuffix', 'token = value', 'AuthORİZation', ' TOken= ', '\u200b', 'safe\x00label']
    for entity,path,label,source in itertools.product(fields,['', ' ', '\u3000', 'root.name','$.item', ' name '],labels,[0,1]):
        cases.append(dict(op='construct',entity=entity,path=path,label=label,source=source))
    for times in [[-7],[0],[123,456],[1,2,3],[0,-1,0,7],[-9223372036854775808,9223372036854775807]]:
        cases.append(dict(op='scope',times=times))
    for pending,polls,outcome in itertools.product(range(4),range(6),['ok','error','panic']):
        cases.append(dict(op='publication',pending=pending,polls=polls,outcome=outcome))
    properties=[dict(name='id',required=True,id=True,version=False),
                dict(name='version',required=True,id=False,version=True),
                dict(name='displayName',required=True,id=False,version=False),
                dict(name='optional',required=False,id=False,version=False)]
    states=[None,value('null'),value('typed_null'),value('text','ok'),value('text','')]
    for identity,name,marker,language in itertools.product(states,states,['create','update'],['en','fr','zh-CN']):
        values={'__teaql_object_status':value('text',marker)}
        if identity is not None: values['id']=identity
        if name is not None: values['displayName']=name
        cases.append(dict(op='payload',properties=properties,values=values,path=['orders',2],entity='Item',language=language))
    for required,identity,version,present in itertools.product([False,True],[False,True],[False,True],[None,value('null'),value('typed_null')]):
        prop=dict(name='field',required=required,id=identity,version=version)
        cases.append(dict(op='payload',properties=[prop],values={} if present is None else {'field':present},path=[],entity='Item',language='en'))
    cases.append(dict(op='payload',properties=properties,values={},path=[],entity='Missing',language='en'))
    return cases


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--rust',required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    cases=inputs(); outputs=observe(args.rust,cases)
    lock=json.loads((root/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],note='Original FixEvidence/Context/private CREATE methods and task-local keys through access bridges; publication/cancellation/panic observations use the original public audited-save wrapper with a delegated saver fixture, not copied orchestration.',
                cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    Path(__file__).with_name('graph_fix_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Recorded {len(cases)} Graph Fix/CREATE observations')


if __name__=='__main__':main()
