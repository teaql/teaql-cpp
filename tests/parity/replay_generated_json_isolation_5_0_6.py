#!/usr/bin/env python3
"""Observe snapshot, graph identity and ledger isolation in unchanged generated models."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / 'rust_field_layout/src/bin/generated_json_isolation_5_0_6.rs'
TARGET = Path(__file__).with_name('generated_json_isolation_5_0_6_cases.json.gz')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    inputs = [dict(case=case) for case in ('batch','owned','independent','views')]
    expected = observe(args.rust, inputs)
    batch, owned, independent, views = expected
    assert batch['sharing'] == [[True,True,False],[True,True,False],[False,False,True]]
    assert batch['dirty'] == [['note'],None,None] and batch['loaded'] == [True,True,False]
    assert owned['same_entity_address'] and owned['children_clean']
    assert not owned['parent_lines_loaded'] and owned['child_order_loaded'] == [True,False]
    assert owned['dirty'] == ['code'] and owned['other_dirty'] is None
    assert independent == dict(shared_snapshot=True, quantities=[1,2], clean=[True,True])
    assert views['root_first_snapshot'] and not views['first_second_snapshot']
    assert views['loaded'] == [True,False] and views['first_empty'] and views['second_missing'] and views['children_clean']
    assert views['detached'] == views['json']['lines'][0]['order']
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  driver=str(DRIVER.relative_to(ROOT)),driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    if args.check:
        with gzip.open(TARGET,'rt') as source:
            assert same(corpus,json.load(source)), 'Original generated isolation observations changed'
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    if args.cpp:
        actual = observe(args.cpp,inputs)
        assert same(expected,actual), 'Generated C++ isolation differs from original Rust'
    print('Reproduced 4 original generated JSON isolation contracts' + (' and C++' if args.cpp else ''))


if __name__ == '__main__':
    main()
