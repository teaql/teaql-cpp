#!/usr/bin/env python3
"""Regenerate observations from the locked public Rust schema provider."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
from compare_values import observe


def prop(name="value", kind=5, **kw):
    return dict(name=name, type=kind, **kw)


def entity(properties=None, name="Sample", table="sample", **kw):
    return dict(name=name, table=table, properties=properties or [prop()], **kw)


def cases():
    out = []
    def add(label, entities, setup=(), **kw):
        out.append(dict(label=label, input=dict(setup=list(setup), rounds=[dict(entities=entities, **kw)], inspect=[e['table'] for e in entities])))
    add("empty metadata", [])
    for kind in range(10):
        add(f"create type {kind}", [entity([prop(kind=kind)])])
    types = ["", "BIGINT", "INTEGER", "INT8", "BOOL", "BOOLEAN", "TEXT", "CLOB", "VARCHAR(10)", "REAL", "FLOAT", "DOUBLE PRECISION", "NUMERIC", "DECIMAL(12,4)", "JSON", "DATE", "TIMESTAMP", "DATETIME", "BLOB", "CUSTOM", "POINT", "INTERVAL", "TEXTINT", "BOOLNUM", "timestamp with time zone", " text ", "\u2003VARCHAR(8)\u2003"]
    for kind in range(10):
        for declared in types:
            for required in [False, True]:
                quoted = '"' + declared.replace('"','""') + '"' if declared else ''
                add(f"family {kind}/{declared}/{required}", [entity([prop(kind=kind, nullable=not required)])], [f"CREATE TABLE sample(value {quoted}{' NOT NULL' if required else ''})"])
    for actual in ["TEXT", "VARCHAR(0)", "VARCHAR(4)", "VARCHAR(20)", "VARCHAR(+20)", "VARCHAR(-1,12)", "VARCHAR(invalid,9)", "VARCHAR(4294967296,8)", "VARCHAR(\u200315\u2003)", "VARCHAR(2,3,4)", "VARCHAR(20", "VARCHAR()"]:
        for length in [0, 4, 9, 20, 100]:
            add(f"length {actual}/{length}", [entity([prop(length=length)])], [f'CREATE TABLE sample(value "{actual}")'])
    for actual in ["NUMERIC", "DECIMAL(12,4)", "DECIMAL(12)", "DECIMAL(0,0)", "DECIMAL(2,5)", "DECIMAL(bad,12,4)", "DECIMAL(+12,+4)", "DECIMAL(4294967296,12,4)"]:
        for precision,scale in [(0,0),(10,2),(12,4),(12,5),(15,2),(2,5)]:
            add(f"numeric {actual}/{precision}/{scale}", [entity([prop(kind=4,precision=precision,scale=scale)])], [f'CREATE TABLE sample(value "{actual}")'])
    for required in [False,True]:
        for pk in [False,True]:
            add(f"nullability {required}/{pk}",[entity([prop(kind=1,nullable=not required)])],[f"CREATE TABLE sample(value BIGINT{' PRIMARY KEY' if pk else ''})"])
    base=entity([prop("id",1,id=True,nullable=False),prop("version",1,version=True,nullable=False),prop("name")])
    for sink in [False,True]:
        for legacy in ["none","initial","root"]:
            out.append(dict(label=f"events {sink}/{legacy}",input=dict(rounds=[dict(entities=[base],sink_error=sink,legacy=legacy),dict(entities=[base])],inspect=["sample","SAMPLE","missing"])))
    for populated in [False,True]:
        for required in [False,True]:
            changed=copy.deepcopy(base);changed['properties'].append(prop("added",nullable=not required))
            out.append(dict(label=f"add column {populated}/{required}",input=dict(rounds=[dict(entities=[base]),dict(entities=[changed],sql=["INSERT INTO sample(id,version) VALUES(1,1)"] if populated else [])],inspect=["sample"])))
    bad=copy.deepcopy(base);bad['properties']=[prop("added"),prop("name",1)]
    out.append(dict(label="rollback retains event",input=dict(rounds=[dict(entities=[base]),dict(entities=[bad])],inspect=["sample"])))
    for table in ["order items",'"Quoted"',"`backtick`","[bracket]","Σchema","é-table"]:
        add(f"quoted table {table}",[entity([prop("id",1,id=True,nullable=False),prop("version",1,version=True,nullable=False)],table=table)])
    for name in ["İΣ","ΟΣ","AΣ'A","AΣ'","ΣA","Straße","名字","A\u0345Σ","𐐀Σ","ÅId",'"quoted"',"[bracket]","space name"]:
        e=entity([prop(name)])
        out.append(dict(label=f"unicode column {name}",input=dict(rounds=[dict(entities=[e]),dict(entities=[e])],inspect=["sample"])))
    for installed in ["CREATE INDEX PK_SAMPLE_ID_VERSION ON sample(id,version)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(version,id)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(id DESC,version)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(id COLLATE NOCASE,version)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(id,version) WHERE version>0","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(id+1,version)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(id)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON sample(id,version)"]:
        add("index shape "+installed,[base],["CREATE TABLE sample(id BIGINT NOT NULL,version BIGINT NOT NULL,name TEXT)",installed])
    add("index global collision",[base],["CREATE TABLE elsewhere(id,version)","CREATE UNIQUE INDEX PK_SAMPLE_ID_VERSION ON elsewhere(id,version)"])
    for logical in ["parentId","createTime","update_time","create_time","when_time","plain","SomeID","İId"]:
        add("index logical name "+logical,[entity([prop(logical,1,column="mapped")])])
    parent=entity([prop("id",1,id=True,nullable=False)],name="Parent",table="parents")
    child=entity([prop("id",1,id=True,nullable=False),prop("parentId",1)],name="Child",table="children",relations=[dict(name="parent",target="Parent",local="parentId",foreign="id")])
    for many in [False,True]:
        for services in [(None,None),("local","local"),(None,"local"),("a","b")]:
            p,c=copy.deepcopy(parent),copy.deepcopy(child)
            if many:
                c.pop('relations');p['relations']=[dict(name="children",target="Child",local="id",foreign="parentId",many=True)]
            if services[0] is not None:p['service']=services[0]
            if services[1] is not None:c['service']=services[1]
            out.append(dict(label=f"FK direction/service {many}/{services}",input=dict(rounds=[dict(entities=[p,c]),dict(entities=[c,p])],inspect=["parents","children"])))
    for mutation in ["missing target","missing local","missing foreign","duplicate","self"]:
        p,c=copy.deepcopy(parent),copy.deepcopy(child)
        if mutation=="missing target":c['relations'][0]['target']='Missing'
        if mutation=="missing local":c['relations'][0]['local']='missing'
        if mutation=="missing foreign":c['relations'][0]['foreign']='missing'
        if mutation=="duplicate":c['relations'].append(copy.deepcopy(c['relations'][0]))
        if mutation=="self":c['relations'][0]['target']='Child'
        add("FK "+mutation,[p,c])
    for actions in ["","ON DELETE CASCADE","ON UPDATE CASCADE","ON UPDATE RESTRICT ON DELETE RESTRICT","ON DELETE SET NULL","ON UPDATE NO ACTION ON DELETE NO ACTION"]:
        add("installed FK "+actions,[parent,child],["CREATE TABLE parents(id BIGINT NOT NULL PRIMARY KEY)",f"CREATE TABLE children(id BIGINT NOT NULL PRIMARY KEY,parentId BIGINT,FOREIGN KEY(parentId) REFERENCES parents(id) {actions})"])
    add("missing FK",[parent,child],["CREATE TABLE parents(id BIGINT NOT NULL PRIMARY KEY)","CREATE TABLE children(id BIGINT NOT NULL PRIMARY KEY,parentId BIGINT)"])
    add("composite FK",[parent,child],["CREATE TABLE parents(id BIGINT NOT NULL PRIMARY KEY,other BIGINT)","CREATE TABLE children(id BIGINT NOT NULL PRIMARY KEY,parentId BIGINT,other BIGINT,FOREIGN KEY(parentId,other) REFERENCES parents(id,other))"])
    for name in ["children","parents"]:
        p,c=copy.deepcopy(parent),copy.deepcopy(child)
        (p if name=='parents' else c)['table']='"'+name+'"'
        add("quoted FK "+name,[p,c])
    soundex=["NULL","123","1.2","X'414243'","CAST(X'FF' AS TEXT)","''","'Robert'","'Rupert'","'Ashcraft'","'123中文É'","'B-bbp'","'Babab'","'Tymczak'","'A' || char(0) || 'b'"]
    add("source soundex registration",[],queries=["SELECT "+','.join('soundex('+x+')' for x in soundex)])
    # BEGIN failure preserves a caller-owned active transaction.
    add("existing transaction",[],["BEGIN IMMEDIATE"],queries=["SELECT soundex('Robert')"])
    return out

if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--rust',required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify_upstream.py')],check=True)
    data=cases();values=observe(args.rust,[c['input'] for c in data])
    for c,v in zip(data,values):c['expected']=v
    path=Path(__file__).with_name('sqlite_schema_cases.json')
    baseline=json.loads((root/'upstream.lock.json').read_text())['commit']
    path.write_text('{"baseline":'+json.dumps(baseline)+',"cases":[\n'+',\n'.join(json.dumps(c,ensure_ascii=True,separators=(',',':')) for c in data)+'\n]}\n')
    print(f'Observed {len(data)} public Rust schema cases')
