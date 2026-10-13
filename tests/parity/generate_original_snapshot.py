#!/usr/bin/env python3
"""Compare original snapshot/clone/ledger calls against unchanged Rust 5.0.6."""
import argparse
import json
from pathlib import Path
import subprocess
from generate_field_layout import observe
ROOT = Path(__file__).resolve().parents[2]

def cases():
    values = [dict(kind='null'), dict(kind='bool', value=False),
              dict(kind='i64', value='0'), dict(kind='text', value=''),
              dict(kind='text', value='雪'), dict(kind='text', value='x'*4096),
              dict(kind='json', value={'nested': [None, False, 0, 'value']})]
    out = []
    for entity in ['Item', '雪']:
        for version in [None, -3, 0, 3]:
            for payload in values:
                first = dict(id=dict(kind='u64', value='7'), payload=payload)
                if version is not None:
                    first['version'] = dict(kind='i64', value=str(version))
                second = dict(id=dict(kind='u64', value='8'), version=dict(kind='i64', value='9'))
                out.append(dict(kind='original', entity=entity, first=first, second=second, dirty=payload))
    return out

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected, actual = observe(args.rust.resolve(), inputs), observe(args.cpp.resolve(), inputs)
    for i,(left,right) in enumerate(zip(expected,actual)):
        if left != right:
            raise SystemExit(f'Original snapshot mismatch {i}:\n{left}\n{right}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'],
        note='Direct original_snapshot, clone, replacement, shared ledger and original version APIs; not complete private ledger-map/governance qualification.',
        cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('original_snapshot_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Compared {len(inputs)} direct Rust original snapshot inputs')

if __name__ == '__main__':
    main()
