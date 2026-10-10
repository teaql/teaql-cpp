"""Record original Checker/TypedChecker/Context calls with handwritten models."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def v(kind, value=None):
    out = dict(kind=kind)
    if value is not None:
        out['value'] = value
    return out


def inputs():
    cases = []
    ids = [None, v('null'), v('typed_null'), v('u64', '0'), v('u64', '7'),
           v('i64', '-1'), v('text', ''), v('bool', False)]
    markers = [None, v('text', 'create'), v('text', 'update'), v('text', 'unknown'),
               v('null'), v('bool', True), v('text', 'CREATE')]
    for identity, marker, status in itertools.product(ids, markers, [None, 0, 1, 2]):
        fields = {}
        if identity is not None: fields['id'] = identity
        if marker is not None: fields['__teaql_object_status'] = marker
        case = dict(op='status', values=fields)
        if status is not None: case['mark'] = status
        cases.append(case)
    texts = [None, v('null'), v('typed_null'), v('i64', '3'), v('bool', True),
             v('text', ''), v('text', 'a'), v('text', '订单😀é'), v('text', 'a\x00b'),
             v('list', []), v('json', {'a': 2})]
    for text, limits, path in itertools.product(texts, [(0,0),(1,1),(2,5),(5,2)],
                                               [[], ['orderItems', 2], ['', 'firstName']]):
        cases.append(dict(op='helpers', values={} if text is None else {'name':text},
                          field='name', min=limits[0], max=limits[1], path=path))
    boundaries = [{}, {'id':v('u64','7')}, {'id':v('null')}, {'name':v('null')},
                  {'id':v('u64','7'),'name':v('null')},
                  {'id':v('u64','7'),'name':v('text','old'),'score':v('i64','3')},
                  {'name':v('text',''), 'score':v('text','2')},
                  {'name':v('typed_null')}, {'score':v('text','bad')},
                  {'name':v('u64','4')}, {'id':v('typed_null'),'score':v('typed_null')},
                  {'version':v('null')}, {'optional':v('null'),'unmapped':v('text','Keep')},
                  {'__teaql_object_status':v('text','create'),'id':v('u64','7')},
                  {'__teaql_object_status':v('text','update'),'name':v('text','old')},
                  {'id':v('u64','7'),'name':v('text','订单😀'),'score':v('i64','3')}]
    actions = ['none','fill','increment','set_id','remove_name','null_name','dirty','report','fill_and_report']
    for fields, action in itertools.product(boundaries, actions):
        cases.append(dict(op='typed', values=fields, action=action, path=['items', 2], language='fr'))
    field_lists = [None, v('null'), v('text','name'), v('list', []),
                   v('list', [v('text','score'),v('text','name'),v('text','name'),v('u64','3')])]
    for loaded, dirty, action, bad in itertools.product(field_lists,field_lists,['none','fill','dirty'],[False,True]):
        fields = {'id':v('u64','7'), 'unmapped':v('text','Keep')}
        if bad: fields['name'] = v('bool', True)
        if loaded is not None: fields['_loaded_fields'] = loaded
        if dirty is not None: fields['_dirty_fields'] = dirty
        cases.append(dict(op='typed', values=fields, action=action, path=[]))
    for fields, typed, metadata, registered, action in itertools.product(boundaries, [False,True],
                                               [False,True],[False,True], ['none','fill','set_id','report','report_twice']):
        cases.append(dict(op='context', values=fields, typed=typed, metadata=metadata,
                          registered=registered, action=action, path=['rootItem'], language='en'))
    for typed, action, replacement, entity, metadata in itertools.product([False,True], ['none','fill','report'],
                                          [None,'none','fill'], ['Item','Missing'], [False,True]):
        case = dict(op='module', values={'id':v('u64','7')}, typed=typed, action=action,
                    entity=entity, metadata=metadata, language='zh-CN')
        if replacement is not None: case['replace'] = replacement
        cases.append(case)
    return cases


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--rust', required=True); args=parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root/'scripts/verify_upstream.py')], check=True)
    cases = inputs(); outputs = observe(args.rust, cases)
    lock = json.loads((root/'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], note='Original public Checker/TypedEntityChecker/registry/UserContext/RuntimeModule calls. Handwritten models supply materialization/projection; runtime algorithms are not copied.',
                  cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    Path(__file__).with_name('checker_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Recorded {len(cases)} checker/fix observations')


if __name__ == '__main__': main()
