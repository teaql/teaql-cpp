#!/usr/bin/env python3
"""Observe generated nullable graph states and exact malformed-input failures."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / 'rust_field_layout/src/bin/generated_json_nullable_graph_5_0_6.rs'
TARGET = Path(__file__).with_name('generated_json_nullable_graph_5_0_6_cases.json.gz')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    holder = [
        {'id': 2, 'record_id': None, 'record': None},
        {'id': 2, 'record_id': 1},
        {'id': 2, 'record': {'id': 1}},
        {'id': 2},
        {'id': 2, 'record': None},
        {'id': 2, 'record_id': None},
        {'id': 2, 'record_id': 1, 'record': {'id': 3}},
        {'id': 2, 'record_id': 1, 'record': None},
        {'id': 2, 'record': []},
        {'id': 2, 'record': {'name': 'PRIVATE_LITERAL'}},
        {'id': 2, 'record_id': None, 'record': {'id': 1}},
    ]
    record = [
        {'id': 1, 'holder_list': []}, {'id': 1},
        {'id': 1, 'holder_list': None},
        {'id': 1, 'holder_list': [None]},
        {'holder_list': []},
        {'id': 1, 'holder_list': [{'id': 1, 'record_id': 999}]},
        {'id': 1, 'holder_list': [{'id': 1, 'name': 123}]},
        {'id': 1, 'holder_list': [{'id': 1, 'record_id': 1, 'record': {'id': 1, 'name': 'PRIVATE_FIRST'}},
                                 {'id': 2, 'record_id': 1, 'record': {'id': 1, 'name': 'PRIVATE_SECOND'}}]},
        {'id': 1, 'type_field': None, 'holder_list': [{'id': 2, 'record': {'id': 1, 'type_field': None}}]},
    ]
    inputs = [dict(entity='Holder', value=v) for v in holder] + [dict(entity='Record', value=v) for v in record]
    expected = observe(args.rust, inputs)
    assert [v['ok'] for v in expected] == [True]*6 + [False]*5 + [True, True] + [False]*6 + [True]
    assert expected[0]['foreign_key'] is None and expected[0]['relation'] == 'null'
    assert expected[1]['foreign_key'] == 1 and expected[1]['relation'] == 'not_loaded'
    assert expected[2]['foreign_key'] == 1 and expected[2]['same_address'] is True
    assert not expected[3]['foreign_key_loaded'] and not expected[3]['relation_loaded']
    assert expected[11]['count'] == 0 and expected[12]['count'] is None
    for result in expected:
        if not result['ok']:
            assert 'PRIVATE_' not in result['error']
        else:
            assert result['clean']
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
    print('Reproduced 20 original generated nullable graph contracts' + (' and C++' if args.cpp else ''))


if __name__ == '__main__':
    main()
