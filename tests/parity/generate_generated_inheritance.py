#!/usr/bin/env python3
"""Compare original flattened inherited model fields, metadata and snapshots."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for entity, loaded, update, text in itertools.product(
            ['school', 'academy', 'research_academy'], [False, True], [False, True],
            ['', 'new', ' \u0085雪\u3000 ', 'x'*1024]):
        yield dict(entity=entity, loaded=loaded, update=update, text=text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    for index, (left, right) in enumerate(zip(expected, actual)):
        if not same(left, right):
            raise SystemExit(f'Inherited model mismatch {index}: {inputs[index]}\nRust: {left}\nC++: {right}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    provenance = json.loads((ROOT/'tests/generator/parity/inheritance_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  note='Unmodified complete original inherited model crate. Two-level scalar/foreign field flattening, exact fixed layouts/revisions, descriptor order/nullability, inherited audit masks, sparse getter/eval/clone/native serialization. C++ qualification-only output; polymorphic query/save gates remain open.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inheritance_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} direct inherited model observations')


if __name__ == '__main__':
    main()
