#!/usr/bin/env python3
"""Compare actual generated-model governed SQLite saves and rollback outcomes."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for status in ['create', 'update']:
        faults = ['none', 'trigger'] if status == 'create' else ['none', 'trigger', 'conflict']
        for root, loaded, name, flags, fault in itertools.product(
                [None, 42, 43], ['all', 'identity', 'no_times', 'no_tenant'],
                ['note', '', ' \u0085value\u3000 '], [(False, False), (True, False), (False, True)], faults):
            reject, fix = flags
            yield dict(mode='commit', status=status, root=root, tenant=0 if status == 'create' else 42,
                       loaded=loaded, name=name, reject=reject, fix=fix, fault=fault)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, left, right) for i, (case, left, right) in enumerate(zip(inputs, expected, actual)) if not same(left, right)]
    for index, case, left, right in failures[:4]:
        print(f'Mismatch {index}: {case}\nRust: {left}\nC++: {right}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} SQLite save observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/checkers_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Public audited-ledger saves using original generated tenant/note entities, Checkers, descriptors and executor. Real SQLite schema, successful create/update, typed rejection, trigger rollback and optimistic conflict; persisted values/version and ledger clearing. Clock epoch values are outside the direct projection. Backend error categories are classified explicitly; exact Check diagnostics and policy plans are retained.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_checker_commit_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} actual SQLite save observations')


if __name__ == '__main__':
    main()
