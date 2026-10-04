#!/usr/bin/env python3
"""Observe actual locked in-memory stores; private bridges tune limits and age."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
def u(n):return dict(kind='u64',value=str(n))
def t(s):return dict(kind='text',value=s)
def cursor(key='query',offset=10,expires=4102444800,**kw):
    out=dict(op='put',key=key,offset=offset,expires=expires,nanos=13,cursor='cursor-'+key,entity='Parent',direction=0,boundary=u(9),page_size=10);out.update(kw);return out
def ids(key='query',values=(1,2,3),expires=4102444800,**kw):
    out=dict(op='put',key=key,ids=list(values),expires=expires,nanos=23);out.update(kw);return out
def cases():
    result=[]
    def add(label,mode,steps,**kw):
        item=dict(mode=mode,namespace='',max_entries=4,max_bytes=64,steps=steps);item.update(kw);result.append(dict(label=label,input=copy.deepcopy(item)))
    # Ordinary reads need a generous scheduling margin, especially under ASan.
    # Expiry itself is tested below using explicitly shifted stored_at values.
    for ns,key,age in itertools.product(['','tenant::a','租户'],['q','tenant::a::entity:Child::q','::','tenant::ab'],[0,100000,2**64-1]):
        add('cache empty/put/replace','cache',[dict(op='get',key=key,age=age),dict(op='put',key=key,rows=[dict(id=u(1),payload=dict(kind='object',value=dict(name=t('nested'))))]),dict(op='get',key=key,age=age),dict(op='put',key=key,rows=[]),dict(op='get',key=key,age=age),dict(op='clear'),dict(op='get',key=key,age=0)],namespace=ns)
    for millis,age in itertools.product([-50000,50000],[0,1,1000,100000,2**64-1]):
        add('cache monotonic age','cache',[dict(op='put',key='q',rows=[dict(n=u(3))]),dict(op='age',key='q',millis=millis),dict(op='get',key='q',age=age),dict(op='get',key='q',age=0)])
    for prefix in ['a','a:','a::','ab','','租户']:
        keys=['a','ab','a:','a::','a::child','ab::child','a::child::leaf','::q','租户::q'];steps=[dict(op='put',key=k,rows=[dict(index=u(i))]) for i,k in enumerate(keys)];steps+=[dict(op='invalidate',key=prefix)];steps.extend(dict(op='get',key=k,age=0) for k in keys);add('cache delimiter invalidation','cache',steps)
    for capacity,replace,expired in itertools.product([0,1,2,4],[False,True],[False,True]):
        values=[cursor('a',expires=0 if expired else 4102444801),cursor('b',expires=4102444802),cursor('a' if replace else 'c',expires=4102444803)]
        values += [dict(op='get',key=k,offset=10) for k in ['a','b','c']];add('cursor eviction', 'cursor',values,max_entries=capacity)
    for key,offset,direction in itertools.product(['q','q:x','','键:值'],[0,1,2**64-1],[0,1]):
        add('cursor roundtrip/offset','cursor',[cursor(key,offset,9223372036854770000,direction=direction,boundary=dict(kind='object',value=dict(id=u(7),label=t('boundary')))),dict(op='get',key=key,offset=offset),dict(op='get',key=key,offset=(offset+1)%(2**64)),dict(op='invalidate',key=key),dict(op='get',key=key,offset=offset)])
    for mode in ['cursor','ids']:
        for expires in [-1,0,1,4102444800,9999999999999]:
            put=cursor('q',expires=expires) if mode=='cursor' else ids('q',expires=expires);get=dict(op='get',key='q',offset=10);add('deadline',mode,[put,get,get])
        for prefix in ['a','a:','ab','','租户']:
            keys=['a','ab','a:x','a:xy','a::x','租户','租户:x',''];put=cursor if mode=='cursor' else ids
            add('invalidation match',mode,[put(key=k,expires=4102444800+i) for i,k in enumerate(keys)]+[dict(op='invalidate',key=prefix)],max_entries=20,max_bytes=4096)
        for cancel in [True,False]:
            put=cursor('q',cancel=cancel) if mode=='cursor' else ids('q',cancel=cancel);add('unpolled future cancellation',mode,[put,dict(op='get',key='q',offset=10),dict(op='invalidate',key='q',cancel=cancel),dict(op='get',key='q',offset=10)])
    for capacity,limit,replace,expired in itertools.product([0,1,2,4],[0,8,16,32,64],[False,True],[False,True]):
        steps=[ids('a',[1,2],expires=0 if expired else 4102444801),ids('b',[2],expires=4102444802),ids('a' if replace else 'c',[7,8],expires=4102444803)];steps.extend(dict(op='get',key=k) for k in ['a','b','c']);add('id capacity/bytes/replacement','ids',steps,max_entries=capacity,max_bytes=limit)
    for length,limit in itertools.product([0,1,2,3,8],[0,8,16,24,64]):
        add('oversized rejection before expiry sweep','ids',[ids('old',[],expires=0),ids('new',range(length)),dict(op='get',key='old'),dict(op='get',key='new')],max_bytes=limit)
    add('id duplicates and large integers','ids',[ids('q',[0,1,1,2**64-1]),dict(op='get',key='q')])
    add('default id capacity','ids',[ids(str(i),[i],expires=4102444800+i) for i in range(65)]+[dict(op='get',key='0'),dict(op='get',key='64')],max_entries=64,max_bytes=256*1024*1024)
    for plan,cursor_id,count in itertools.product(['','SEEK','ID_SET_HIT','状态'],[None,'','opaque-id'],[None,0,2**64-1]):
        add('Context observations','observations',[dict(op='read'),dict(op='continuous',key=plan,cursor=cursor_id),dict(op='ids',key=plan,count=count),dict(op='continuous',key='replacement',cursor=None),dict(op='ids',key='replacement',count=None)])
    return result
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);args=p.parse_args();root=Path(__file__).resolve().parents[2];subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    data=cases();outputs=observe(args.rust,[c['input'] for c in data]);assert len(outputs)==len(data)
    for c,out in zip(data,outputs):c['expected']=out
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit'];Path(__file__).with_name('query_store_cases.json').write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,separators=(',',':')) for c in data)+'\n]}\n');print(f'Observed {len(data)} locked query-store cases')
