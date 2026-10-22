#!/usr/bin/env python3
"""Observe generated nullable children, repeated parents and private root mutations."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / 'rust_field_layout/src/bin/generated_json_owned_nullable_5_0_6.rs'
TARGET = Path(__file__).with_name('generated_json_owned_nullable_5_0_6_cases.json.gz')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = [dict(case='owned_nullable')]
    expected = observe(args.rust, inputs)
    [row] = expected
    assert row['child_values'] == ['first', None]
    assert row['child_scalar_loaded'] == [True, True]
    assert row['child_parent_loaded'] == [True, False]
    assert row['children_clean'] and row['root_clean'] and row['same_address']
    assert row['parent_name'] == 'parent' and not row['parent_reverse_loaded']
    assert row['root_dirty'] == ['name'] and row['other_dirty'] is None
    assert row['json'] == {'id': 1, 'version': 7, 'name': 'parent', 'record_list': [
        {'id': 1, 'version': 7, 'type_field': 'first', 'platform_id': 1,
         'platform': {'id': 1, 'version': 7, 'name': 'parent'}},
        {'id': 2, 'version': 7, 'type_field': None, 'platform_id': 1}]}
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  cases=[dict(input=i, expected=o) for i, o in zip(inputs, expected)])
    if args.check:
        with gzip.open(TARGET, 'rt') as source:
            assert same(corpus, json.load(source)), 'Original nullable graph observations changed'
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    if args.cpp:
        assert same(expected, observe(args.cpp, inputs)), 'Generated nullable graphs differ'
    print('Reproduced 1 original generated nullable owned graph contract' + (' and C++' if args.cpp else ''))


if __name__ == '__main__':
    main()
