#!/usr/bin/env python3
"""Record actual original generated membership construction and SQLite outcomes."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    profiles = [(), ('child_deleted',), ('child_slice',), ('child_group',),
                ('child_relations',), ('child_options',), ('repeat',),
                ('child_options', 'child_relations', 'outer_deleted', 'repeat')]
    for outer, child, outer_minimal, child_minimal, alias in itertools.product(
            ['person', 'order'], ['person', 'order', 'product'], [False, True], [False, True], [False, True]):
        for profile in profiles:
            yield dict(outer=outer, child=child, outer_minimal=outer_minimal,
                       child_minimal=child_minimal, alias=alias, cutoff=10,
                       **{key: True for key in profile})
    for outer, child, child_minimal, alias, cutoff, child_slice in itertools.product(
            ['person', 'order'], ['person', 'order', 'product'], [False, True],
            [False, True], [0, 10, 100], [False, True]):
        yield dict(outer=outer, child=child, outer_minimal=True, child_minimal=child_minimal,
                   alias=alias, cutoff=cutoff, child_slice=child_slice, execute=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    inputs = list(cases())
    expected = observe(args.rust, inputs)
    actual = observe(args.cpp, inputs)
    errors = [(i, entry, original, cpp) for i, (entry, original, cpp)
              in enumerate(zip(inputs, expected, actual)) if not same(original, cpp)]
    for i, entry, original, cpp in errors[:3]:
        print(f'Mismatch {i}: {entry}\nRust: {original}\nC++: {cpp}')
    if errors:
        raise SystemExit(f'{len(errors)}/{len(inputs)} matching-any observations differ')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    producer = json.loads((ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
        note='Actual unchanged generated matching_any_of/match_any_of, complete query Debug, child immutability and real existing SQLite execution; no output normalization or bootstrap claim.',
        cases=[dict(input=entry, expected=result) for entry, result in zip(inputs, expected)])
    path = Path(__file__).with_name('generated_matching_any_5_0_6_cases.json.gz')
    path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} original/C++ observations; {sum(bool(i.get("execute")) for i in inputs)} SQLite executions')


if __name__ == '__main__':
    main()
