#!/usr/bin/env python3
"""Observe inherited optional forward graphs with the frozen original runtime."""
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
    for model, entry, identity, operation in itertools.product(
            ['school', 'academy', 'research_academy'], ['normal', 'minimal', 'children'],
            [7, 8, 9], ['read', 'update', 'clear', 'delete', 'related_update', 'related_before']):
        faults = ['none'] if operation == 'read' else [
            'none', 'root_conflict', 'advisor_conflict', 'root_trigger', 'advisor_trigger']
        for fault in faults:
            yield dict(model=model, entry=entry, id=identity, operation=operation, fault=fault)
    for entry, selection, identity, operation in itertools.product(
            ['normal', 'minimal', 'children'], ['none', 'normal', 'minimal', 'nested', 'filtered'],
            [7, 8, 9], ['read', 'root_update', 'children_before', 'children_after', 'children_no_attach', 'children_delete']):
        faults = ['none'] if operation == 'read' else [
            'none', 'root_conflict', 'child_conflict', 'root_trigger', 'child_trigger', 'second_trigger']
        for fault in faults:
            yield dict(direction='reverse', entry=entry, selection=selection, id=identity, operation=operation, fault=fault)
    for model, entry, identity, shortcut, operation in itertools.product(
            ['school', 'academy', 'research_academy'], ['normal', 'minimal', 'children'],
            [7, 8, 9], ['self_without_parent', 'all', 'children', 'any'], ['read', 'update']):
        yield dict(model=model, entry=entry, id=identity, shortcut=shortcut, operation=operation, fault='none')


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
    failures = [(i, case, a, b) for i, (case, a, b) in
                enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:3]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} inherited graph observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/inheritance_graph_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Complete unchanged original inheritance graph crate. Forward/nested/reverse views and actual audited ledger saves, nullable/dangling IDs, explicit member state adoption, update/delete/rollback and 216 projection/relation shortcut cases. Exact edge views preserve nested descendants after identity reload. Full strings are retained except explicit conflict/fixture classifications. Remaining inherited configurations stay open.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inheritance_graph_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} inherited graph observations')


if __name__ == '__main__':
    main()
