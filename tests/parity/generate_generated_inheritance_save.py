#!/usr/bin/env python3
"""Compare audited saves of actual original inherited/returned models."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS_NOTE = ('Actual original generated runtime/request/model plus public audited ledger save. '
               'Source/result identities, original version/snapshot, native values, Checker diagnostics, '
               'all four physical tables, pending ledger and saved readback. Conflict and injected '
               'trigger failures are explicit classifications; other errors retain their full strings. '
               'Includes related attachment failures, inherited creation, successful secondary-row '
               'updates, inherited deletion and transaction rollback. Broader nested graph shapes remain open.')


def cases():
    models = ['school', 'academy', 'research_academy']
    for source, target, entry, fault, name, own_fields, related in itertools.product(
            models, models, ['normal', 'minimal', 'children'],
            ['none', 'conflict', 'trigger'], ['updated', '', 'x'*101, ' \u0085雪\u3000 '], [False, True], [False, True]):
        if entry == 'minimal' and related:
            continue
        yield dict(source=source, target=target, entry=entry, fault=fault, name=name, id=7, own_fields=own_fields, related=related)
    for target, entry, fault in itertools.product(models, ['normal', 'minimal', 'children'], ['none', 'conflict', 'trigger']):
        yield dict(source='school', target=target, entry=entry, fault=fault, name='updated', id=9)
    for target, secondary, fault, name in itertools.product(
            models, ['none', 'new', 'existing'], ['none', 'root_trigger', 'related_trigger'], ['created', '', 'x'*101]):
        yield dict(operation='create', target=target, secondary=secondary, fault=fault, name=name)
    for source, target, entry, fault, related in itertools.product(
            models, models, ['normal', 'minimal', 'children'], ['none', 'conflict', 'trigger'], [False, True]):
        if entry == 'minimal' and related:
            continue
        yield dict(operation='delete', source=source, target=target, entry=entry, fault=fault, related=related, id=7)
    for target, entry, fault in itertools.product(models, ['normal', 'minimal', 'children'], ['none', 'conflict', 'trigger']):
        yield dict(operation='delete', source='school', target=target, entry=entry, fault=fault, id=9)


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
    failures = [(i, case, left, right) for i, (case, left, right) in
                enumerate(zip(inputs, expected, actual)) if not same(left, right)]
    for index, case, left, right in failures[:3]:
        print(f'Mismatch {index}: {case}\nRust: {left}\nC++: {right}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} inherited save observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    provenance = json.loads((ROOT/'tests/generator/parity/inheritance_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  note=CORPUS_NOTE,
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inheritance_save_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual inherited save observations')


if __name__ == '__main__':
    main()
