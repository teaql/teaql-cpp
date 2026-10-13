#!/usr/bin/env python3
"""Compare generated generic operators, independent criteria and range helpers."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
FIELDS = [('person','id'),('person','name'),('person','email'),('order','id'),('order','code'),('order','priority')]
OPS = ['eq','ne','gt','gte','lt','lte','between','in','not_in','contain','not_contain','begin','not_begin','end','not_end','sound','null','not_null']


def v(kind, value=None):
    return dict(kind=kind, **({'value':value} if value is not None else {}))


def cases():
    samples = [[],[v('text','Alpha')],[v('text',"%_\\'你好")],[v('i64','10')],
               [v('null')],[v('typed_null')],[v('bool',False)],
               [v('text','Alpha'),v('text','Beta')],[v('i64','10'),v('i64','20')],
               [v('text','Alpha'),v('i64','10'),v('null')],
               [v('json',{'a':1})],[v('date','2026-10-13'),v('timestamp','123')],
               [v('decimal','1.25')], [v('u64','18446744073709551615')],
               [v('f64','-0.0')], [v('list',[v('text','Beta'),v('i64','10')])],
               [v('object',{'answer':v('i64','4')})]]
    for (entity, field), minimal, route, operator, values in itertools.product(
            FIELDS, (False,True), ('direct','criteria'), OPS, samples):
        yield dict(entity=entity,field=field,minimal=minimal,route=route,
                   operator=operator,values=values)
    for (entity, field), minimal, route, operator in itertools.product(
            FIELDS,(False,True),('direct','criteria'),('eq','in','contain','between')):
        yield dict(entity=entity,field=field,minimal=minimal,route=route,
                   operator=operator,values=[v('text','Alpha'),v('text','Beta')],repeat=True)
    for (entity, field), minimal, route, operator, count in itertools.product(
            FIELDS, (False, True), ('direct', 'criteria'), ('in', 'not_in'), (20, 21, 100)):
        yield dict(entity=entity, field=field, minimal=minimal, route=route,
                   operator=operator, values=[v('i64', str(i)) for i in range(count)])
    ranges = [[v('text','Alpha'),v('text','Beta')],[v('i64','10'),v('i64','20')],
              [v('date','2026-01-01'),v('date','2026-12-31')],
              [v('timestamp','-1'),v('timestamp','123')], [v('null'),v('typed_null')],
              [v('decimal','1.25'),v('decimal','3.5')]]
    for (entity, field), minimal, route, values in itertools.product(
            [item for item in FIELDS if item[1]!='id'], (False,True),
            ('before','after','range'),ranges):
        yield dict(entity=entity,field=field,minimal=minimal,route=route,
                   operator='eq',values=values)
    for (entity, field), minimal, route, operator in itertools.product(
            FIELDS,(False,True),('direct','criteria'),OPS):
        values = [v('i64','10'),v('i64','20')] if field in ('id','priority') else [v('text','Alpha'),v('text','Beta')]
        yield dict(entity=entity,field=field,minimal=minimal,route=route,
                   operator=operator,values=values,execute=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True)
    parser.add_argument('--cpp',required=True)
    args = parser.parse_args()
    entries = list(cases())
    original = observe(args.rust,entries)
    cpp = observe(args.cpp,entries)
    errors = [(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
    for i,entry,a,b in errors[:3]:
        print(f'Mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
    if errors:
        raise SystemExit(f'{len(errors)}/{len(entries)} field criteria observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'],version=lock['version'],producer=producer,
                  note='Actual unchanged generated operators, associated criterion factories and DateRange/before/after helpers. Full Expr/SelectQuery Debug, panic diagnostics and existing SQLite execution, without normalization. C++ bound criteria use checked Context object IDs.',
                  cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    target = Path(__file__).with_name('generated_field_criteria_5_0_6_cases.json.gz')
    target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    executed = [(entry,result) for entry,result in zip(entries,original) if entry.get('execute')]
    print(f'Compared {len(entries)} field criteria observations; {len(executed)} requested executions, '
          f'{sum("rows" in result for _,result in executed)} reached execution, '
          f'{sum("rows" in result and result["error"] is None for _,result in executed)} successful')


if __name__ == '__main__':
    main()
