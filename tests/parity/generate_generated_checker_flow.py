#!/usr/bin/env python3
"""Compare same-model original Checker preflight and governed policy flow."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for mode in ['direct', 'governed']:
        for status, root, tenant, loaded, name, flags in itertools.product(
                ['create', 'update', 'unknown'] if mode == 'direct' else ['create', 'update'],
                [None, 42, 43], [0, 42, 43], ['all', 'identity', 'no_times', 'no_tenant'],
                ['note', '', '\u0085\u00a0\u3000'], [(False, False), (True, False), (False, True)]):
            reject, fix = flags
            yield dict(mode=mode, status=status, root=root, tenant=tenant, loaded=loaded,
                       name=name, reject=reject, fix=fix)


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
        raise SystemExit(f'{len(failures)} / {len(inputs)} Checker flow observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/checkers_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Unmodified complete original tenant/note generated crate. Direct Checker and public audited-ledger preflight/policy calls, Context root/time fixes, custom logic ordering and exact diagnostics/evidence. Clock epoch milliseconds and execution IDs are outside the explicit projection; clock field presence, before/after sentinel changes and common operation-clock equality are observed directly. Governed policy denies before persistence; successful native SQLite save supplies separate integration evidence. No output normalization.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_checker_flow_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(inputs)} direct generated Checker flow observations')


if __name__ == '__main__':
    main()
