#!/usr/bin/env python3
"""Observe two generated relationships to the same identity and inferred-key conflicts."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / 'rust_field_layout/src/bin/generated_json_ambiguous_5_0_6.rs'
TARGET = Path(__file__).with_name('generated_json_ambiguous_5_0_6_cases.json.gz')
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    inputs = [dict(case=case) for case in ('ambiguous','inferred_conflict')]
    expected = observe(args.rust,inputs)
    first, second = expected
    assert first['first_loaded'] and not first['second_loaded'] and first['ambiguous']
    assert first['first_json'] == first['json']['first_record']
    assert first['second_json'] == first['json']['second_record']
    assert 'conflicting native values for one identity' in second['error']
    corpus = dict(baseline=lock['commit'],version=lock['version'],driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    if args.check:
        with gzip.open(TARGET,'rt') as source:
            assert same(corpus,json.load(source)), 'Original ambiguous view observations changed'
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    if args.cpp:
        assert same(expected,observe(args.cpp,inputs)), 'C++ ambiguous view semantics differ'
    print('Reproduced 2 original generated JSON ambiguous-view contracts' + (' and C++' if args.cpp else ''))
if __name__ == '__main__':
    main()
