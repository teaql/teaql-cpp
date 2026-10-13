#!/usr/bin/env python3
"""Compare branched inheritance and inherited scalar/clock behavior."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    models = ['document', 'intermediate_record', 'invoice', 'receipt', 'special_invoice']
    for model, entry, mode, automatic, manual, subtype in itertools.product(
            models, ['normal', 'minimal', 'children'], ['construct', 'read'],
            [False, True], [False, True], [False, True]):
        yield dict(model=model, entry=entry, mode=mode, automatic=automatic, manual=manual, subtype=subtype)
    for model, entry, fault, name in itertools.product(
            models, ['normal', 'minimal', 'children'], ['none', 'conflict', 'trigger'], ['changed', '', 'x'*101]):
        yield dict(model=model, entry=entry, mode='save', automatic=False, manual=False, subtype=False, fault=fault, name=name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, a, b) for i, (case, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:2]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} inherited variant observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/inheritance_variants_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Complete unchanged original branched inheritance crate. Empty intermediate, sibling and third-level descendants, compatible field redeclaration, inherited boolean/optional/time fields, recursive/explicit query enhancement, subtype failures and real audited saves. Volatile clock epochs are deliberately observed as changed/not-changed booleans; persistent scalar values and exact diagnostics/errors remain observed. Inherited Business-ID original compilation failure is a separate fixture and is not qualified here.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inheritance_variants_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} inherited variant observations')


if __name__ == '__main__':
    main()
