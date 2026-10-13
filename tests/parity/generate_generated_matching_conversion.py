#!/usr/bin/env python3
"""Compare actual membership conversions and independent generated domains."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same
from generate_generated_matching_any import cases

ROOT = Path(__file__).resolve().parents[2]


def inputs():
    for conversion in ('selection', 'query'):
        for entry in cases():
            yield dict(entry, conversion=conversion)
    for outer, outer_minimal, child_minimal, alias, conversion, profile in itertools.product(
            ('person', 'order'), (False, True), (False, True), (False, True),
            ('request', 'selection', 'query'), ('plain', 'options', 'relations', 'repeat')):
        yield dict(outer=outer, child='school', outer_minimal=outer_minimal,
                   child_minimal=child_minimal, alias=alias, conversion=conversion,
                   cutoff=10, child_options=profile == 'options',
                   child_relations=profile == 'relations', repeat=profile == 'repeat')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    entries = list(inputs())
    original = observe(args.rust, entries)
    cpp = observe(args.cpp, entries)
    failures = [(i, entry, a, b) for i, (entry, a, b) in enumerate(zip(entries, original, cpp))
                if not same(a, b)]
    for i, entry, a, b in failures[:3]:
        print(f'Mismatch {i}: {entry}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)}/{len(entries)} conversion observations differ')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    provenance = {
        fixture: json.loads((ROOT / 'tests/generator/parity' / fixture / 'PROVENANCE.json').read_text())
        for fixture in ('dynamic_json_rust', 'relations_rust')}
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  note='Actual original request/QuerySelection/SelectQuery conversion and cross-crate School requests; full Debug, child immutability and original existing-database outcomes, without normalization.',
                  cases=[dict(input=entry, expected=result) for entry, result in zip(entries, original)])
    target = Path(__file__).with_name('generated_matching_conversion_5_0_6_cases.json.gz')
    target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    executed = [v for entry, v in zip(entries, original) if entry.get('execute')]
    print(f'Compared {len(entries)} original/C++ conversion observations; '
          f'{len(executed)} execution attempts, '
          f'{sum(v["error"] is None for v in executed)} successful, '
          f'{sum(len(v["rows"]) for v in executed)} rows')


if __name__ == '__main__':
    main()
