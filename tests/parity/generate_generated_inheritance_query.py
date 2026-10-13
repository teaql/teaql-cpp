#!/usr/bin/env python3
"""Compare unchanged original generated inheritance query construction."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    types = [None, [], [{'kind': 'text', 'value': 'Academy'}],
             [{'kind': 'text', 'value': 'ResearchAcademy'}, {'kind': 'text', 'value': 'Academy'}],
             [{'kind': 'null'}, {'kind': 'i64', 'value': '-1'}, {'kind': 'bool', 'value': False}]]
    for entity, entry, subtype, group, limit, unlimited, enhance, values in itertools.product(
            ['school', 'academy', 'research_academy'], ['normal', 'minimal', 'children'],
            [False, True], [False, True], [False, True], [False, True],
            ['none', 'automatic', 'manual'], types):
        if entity == 'research_academy' and subtype:
            continue
        yield dict(entity=entity, entry=entry, subtype=subtype, group=group,
                   limit=limit, unlimited=unlimited, enhance=enhance, types=values)
    for entity, entry, shortcut, repeat, enhance in itertools.product(
            ['school', 'academy', 'research_academy'], ['normal', 'minimal', 'children'],
            ['self_without_parent', 'all', 'children', 'any'], [False, True], ['none', 'automatic']):
        yield dict(entity=entity, entry=entry, shortcut=shortcut, repeat=repeat,
                   enhance=enhance, subtype=False, group=False, limit=False, unlimited=False, types=None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, left, right) for i, (case, left, right) in
                enumerate(zip(inputs, expected, actual)) if not same(left, right)]
    for index, case, left, right in failures[:2]:
        print(f'Mismatch {index}: {case}\nRust: {left}\nC++: {right}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} inherited query observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    provenance = json.loads((ROOT/'tests/generator/parity/inheritance_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  note='Direct original query construction: 1800 inherited filter/slice/enhancement cases plus 144 projection/relation shortcut cases. Ordered projections, source duplicates, Expr/Slice Debug and recursive child trees are retained. Shortcut cases additionally observe ordered relation/query trees. Full facets/readonly-definition Debug remains unqualified.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inheritance_query_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} direct inherited query observations')


if __name__ == '__main__':
    main()
