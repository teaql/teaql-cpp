#!/usr/bin/env python3
"""Record actual published Rust evaluation calls and serde behavior."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    states = ['NotLoaded', 'FullyLoaded', {'NotLoaded': None}, {'FullyLoaded': None}]
    for names in [[], ['name'], ['name', 'name', 'company'],
                  ['field'+str(i) for i in range(9)], ['雪', '', 'quote"\\', '\u0000']]:
        states.extend({variant: names} for variant in ['Partial', 'PartialCompact', 'SharedColumns'])
    states.extend([None, 1, 'Unknown', {}, {'Unknown': []}, {'NotLoaded': []},
                   {'Partial': None}, {'Partial': [1]}, {'Partial': [], 'FullyLoaded': None}])
    results = ['Null', {'Null': None}, {'Value': ''}, {'Value': 'teaql'},
               {'Value': '雪🧊\u0000\u2028'}, {'Value': 'x'*8192},
               {'NotLoaded': {'failed_node': 'name', 'attempted_path': 'platform.company.name'}},
               {'NotLoaded': ['node', 'path']},
               {'NotLoaded': {'failed_node': '', 'attempted_path': '', 'extra': 'ignored'}},
               None, 1, 'Unknown', {}, {'Value': 1}, {'Null': []},
               {'NotLoaded': {'failed_node': 'name'}},
               {'NotLoaded': {'failed_node': None, 'attempted_path': 'name'}},
               {'NotLoaded': ['node', 'path', 'extra']}, {'Value': 'v', 'Null': None}]
    cases=[]
    def add(state, result, next='missing', field='platform', node='name', path='company.name', other=None):
        cases.append(dict(state=state, other_state=state if other is None else other,
                          fields=['name','company','','雪','field8','missing'],
                          result=result, next=next, field=field, node=node, path=path))
    for state in states:
        for result in results:
            add(state,result)
    for field in ['', 'name', 'platform', '雪', 'a.b']:
        for path in ['', field, 'name', 'company.name', '雪\u0000']:
            for next in ['value', 'null', 'missing']:
                add({'PartialCompact': ['name']}, {'Value': '雪'}, next, field, 'leaf', path)
    equality_states = ['NotLoaded', 'FullyLoaded']
    for names in [[], ['name'], ['name','company'], ['company','name'],
                  ['name','name','company'], ['name','company','name'], ['雪','']]:
        equality_states.extend({variant:names} for variant in ['Partial','PartialCompact','SharedColumns'])
    for left in equality_states:
        for right in equality_states:
            add(left, 'Null', other=right)
    outputs=observe(args.rust,cases)
    corpus=dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
                note='Unchanged published eval module: all five LoadState variants, set/sequence distinctions, serde acceptance, Clone equality, EvalResult wire variants, consuming callbacks, short circuits, path composition and four Debug modes. HashSet order is normalized for JSON; multiple-element HashSet Debug has no fixed source order.',
                cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    encoded=(json.dumps(corpus,separators=(',',':'))+'\n').encode()
    Path(__file__).with_name('eval_cases.json.gz').write_bytes(gzip.compress(encoded,mtime=0))
    print(f'Recorded {len(cases)} original evaluation inputs')


if __name__ == '__main__':
    main()
