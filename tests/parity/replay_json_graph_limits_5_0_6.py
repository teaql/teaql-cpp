#!/usr/bin/env python3
"""Compare original JSON graph node-limit ordering with the C++ Context entry."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
TARGET = Path(__file__).with_name('json_graph_limits_5_0_6_cases.json.gz')
DRIVER = Path(__file__).parent / 'rust_private_save_5_0_6/src/bin/json_hydration_5_0_6.rs'


def inputs():
    bad = {'unknown_child_field': 'must not be decoded'}
    shapes = [(100000, bad, False), (99999, bad, False), (100001, bad, False),
              (100000, 0, False), (99999, 0, False),
              (99999, bad, True), (99998, bad, True)]
    for count, item, nested in shapes:
        value = {'id': 2, 'children': [item] * count}
        if nested:
            value = {'id': 1, 'parent': value}
        yield dict(kind=2, metadata=0, registered=True, array=False, value=value)


    for rows in (
        [{'id': 2}, {'id': 2, 'label': 'first'}, {'id': 2, 'label': 'second'}],
        [{'id': 2}, {'id': 2, 'label': 'first'}, {'id': 2, 'label': 'first'}],
        [{'id': 2, 'label': 'first'}, {'id': 2}, {'id': 2, 'label': 'second'}],
        [{'id': 2, 'label': 'first'}, {'id': 2}, {'id': 2}],
    ):
        yield dict(kind=2, metadata=0, registered=True, array=False,
                   value={'id': 1, 'children': rows})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    stimuli = list(inputs())
    expected = observe(args.rust, stimuli)
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  cases=[dict(input=i, expected=o) for i, o in zip(stimuli, expected)])
    for index in (0, 2, 3, 5):
        assert 'graph input exceeds depth or node limit' in expected[index]['error']
        assert 'unknown_child_field' not in expected[index]['error']
    for index in (1, 6):
        assert 'unknown model field: unknown_child_field' in expected[index]['error']
        assert 'must not be decoded' not in expected[index]['error']
    assert 'relation item must be an entity object' in expected[4]['error']
    for index in (7, 9):
        assert 'conflicting native values for one identity' in expected[index]['error']
    for index in (8, 10):
        assert 'row' in expected[index]
    if args.check:
        with gzip.open(TARGET, 'rt') as source:
            assert same(corpus, json.load(source)), 'Original graph-limit corpus changed'
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus) + '\n').encode(), mtime=0))
    if args.cpp:
        actual = observe(args.cpp, stimuli, arguments=['--large-fixture'])
        assert same(expected, actual), 'C++ graph-limit error ordering differs'
    print('Reproduced 11 original JSON graph boundary/view cases' +
          (' and C++' if args.cpp else ''))


if __name__ == '__main__':
    main()
