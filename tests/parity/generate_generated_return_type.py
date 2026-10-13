#!/usr/bin/env python3
"""Compare original return_type source/result separation against SQLite."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    models = ['school', 'academy', 'research_academy']
    inputs = [dict(entity=entity, return_type=target, entry=entry, automatic=automatic,
                   manual=manual, limit=limit, result=result)
              for entity, target, entry, automatic, manual, limit, result in itertools.product(
                  models, models, ['normal', 'minimal', 'children'], [False, True],
                  [False, True], [False, True], ['entities', 'one', 'stream'])]
    inputs.extend(dict(entity=entity, return_type=target, entry='normal', automatic=False,
                       manual=False, limit=False, result=result, failure=failure)
                  for entity, target, result, failure in itertools.product(
                      models, models, ['entities', 'one', 'stream'], ['type_filter', 'type_group']))
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, left, right) for i, (case, left, right) in
                enumerate(zip(inputs, expected, actual)) if not same(left, right)]
    for index, case, left, right in failures[:3]:
        print(f'Mismatch {index}: {case}\nRust: {left}\nC++: {right}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} return-type observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    provenance = json.loads((ROOT/'tests/generator/parity/inheritance_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  note='Actual original generated return_type and SQLite list/one/stream execution (stream chunks of one). Source/return models vary independently across all three inheritance levels. Exact result model identity, native values, loaded fields and original snapshots; normal/minimal/with-children, recursive/explicit enhancements and slicing. Generated automatic bootstrap is adapted by explicit C++ root setup, as in the earlier SQLite corpus. Failure and graph-save matrices remain open.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_return_type_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual source/return-model SQLite observations')


if __name__ == '__main__':
    main()
