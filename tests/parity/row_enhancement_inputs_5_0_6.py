"""Declarative inputs for checked row enhancement and transient shape passes."""
from copy import deepcopy
import itertools


def scalar(kind, value=None):
    return dict(kind=kind, value=value)


def dynamic_pairs():
    base=dict(owner='Row',revision='r1',type=5,binding=None,value=scalar('null'))
    bound=dict(base,binding=dict(profile='private-a',executor=7))
    return [(None,None),(base,base),(base,dict(base,owner='Other')),
            (base,dict(base,revision='r2')),(base,dict(base,type=0)),(bound,base),
            (bound,dict(bound,binding=dict(profile='private-b',executor=7))),
            (bound,dict(bound,binding=dict(profile='private-a',executor=8))),
            (bound,bound)]


def cases():
    output=[]
    schema_pairs=[(2,2),(2,3),(2,4),(0,2),(1,2),(2,0),(2,1),(2,5),(0,0)]
    for schema_pair,mode,provenance,reverse in itertools.product(schema_pairs,range(8),range(9),(False,True)):
        suffix='雪\0' if reverse else ''
        first,second='_alpha'+suffix,'_omega'+suffix
        schemas=[None,[],[[first,1],[second,1]],[[first,5],[second,1]],
                 [[first,1],[second,0]],[['_extra',0]]]
        names=['id',first,second]
        left_values=[scalar('u64','7'),scalar('i64','1'),scalar('i64','2')]
        right_values=[scalar('u64','99'),scalar('i64','9'),scalar('i64','8')]
        left_names=names.copy();right_names=names.copy()
        if mode in (1,2):left_values[1]=scalar('text','invalid-left')
        if mode==1:right_names=right_names[:1];right_values=right_values[:1]
        if mode in (3,4,7):right_values[1]=scalar('text','invalid-right')
        if mode==4:
            left_values[2]=scalar('text','invalid-left-second')
            right_names=right_names[:2];right_values=right_values[:2]
        if mode==5:
            right_names=['id',first,first,second]
            right_values=[scalar('u64','99'),scalar('bool',False),scalar('i64','9'),scalar('i64','8')]
        if mode==6:
            left_names=['id',first,first,second]
            left_values=[scalar('u64','7'),scalar('text','overridden'),scalar('i64','11'),scalar('i64','2')]
            right_names=right_names[:2];right_values=right_values[:2]
        if mode==7:right_values[2]=scalar('text','invalid-right-second')
        if reverse:
            left_names.reverse();left_values.reverse();right_names.reverse();right_values.reverse()
        dl,dr=dynamic_pairs()[provenance]
        left=dict(names=left_names,values=left_values,schema=schemas[schema_pair[0]],dynamic=dl,relation=True)
        right=dict(names=right_names,values=right_values,schema=schemas[schema_pair[1]],dynamic=dr,relation=True)
        output.append(dict(kind='merge',left=left,right=right,
                           coverage=dict(schemas=list(schema_pair),values=mode,provenance=provenance,reverse=reverse)))
    for geometry,profile,reverse,target in itertools.product(range(3),range(3),(False,True),(False,True)):
        rows=[]
        for i in range(4):
            names=['id','lines'] if geometry==0 else ['id','lines','lines'] if geometry==1 else []
            values=[scalar('u64',str(i+1)),scalar('text','first'+str(i)),scalar('text','duplicate'+str(i))][:len(names)]
            if reverse:names.reverse();values.reverse()
            schemas=[None,None,None,None] if profile==0 else [None,[],None,[]] if profile==1 else [[['_declared',1]],[],[['_declared',1]],None]
            rows.append(dict(names=names,values=values,schema=schemas[i],dynamic=None,relation=i==1))
        output.append(dict(kind='shapes',rows=rows,schema=[['_rank',1]] if target else [],
                           coverage=dict(geometry=geometry,profile=profile,reverse=reverse,target=target)))
    pool=[scalar('null'),scalar('typed_null'),scalar('bool',False),scalar('bool',True),
          scalar('i64','-9223372036854775808'),scalar('i64','9223372036854775807'),
          scalar('u64','18446744073709551615'),scalar('f64','-0.0'),scalar('f64','NaN'),
          scalar('f64','inf'),scalar('f64','-inf'),scalar('decimal','-1234567890.00100'),
          scalar('text','text\0雪\n"\\'),scalar('json',{'nested':[None,False,'雪']}),
          scalar('date','2032-02-29'),scalar('timestamp','-9223372036854775808'),
          scalar('timestamp','9223372036854775807'),
          scalar('list',[scalar('typed_null'),scalar('text','nested\0')]),
          scalar('object',{'x':scalar('list',[scalar('null'),scalar('u64','9')])}),
          scalar('text','long'*1024)]
    output.append(dict(kind='record',row=dict(names=[],values=[],schema=None,dynamic=None,relation=False)))
    for payload,names in itertools.product(pool,(['one'],['','雪\0','same','same'])):
        values=[payload] if len(names)==1 else [scalar('null'),payload,scalar('bool',False),payload]
        output.append(dict(kind='record',row=dict(names=names,values=values,schema=None,dynamic=None,relation=False)))
    return deepcopy(output)
