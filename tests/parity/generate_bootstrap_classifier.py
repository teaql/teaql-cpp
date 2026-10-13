#!/usr/bin/env python3
"""Compare the exact original generated retry classifier against shared C++."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/generate_bootstrap_classifier_reference.py'), '--check'], check=True)
    messages = ['', 'UNIQUE constraint failed: status_data.id',
                'UNIQUE constraint failed: status_data.code', 'UNIQUE constraint failed: status.id',
                'unique constraint failed: status_data.id',
                'prefix UNIQUE constraint failed: status_data.id suffix',
                'UNIQUE constraint failed: status_data.identifier',
                'UNIQUE constraint failed: status_data.id_other',
                'duplicate key value violates unique constraint status_data_pkey',
                'duplicate key value violates unique constraint other_key',
                'Duplicate entry 1 for PRIMARY', 'Duplicate entry 1 for secondary',
                'PRIMARY Duplicate entry', 'constraint failed', 'UNIQUE constraint failed:',
                '_data.id UNIQUE constraint failed:', '\nUNIQUE constraint failed: status_data.id\n']
    inputs = [dict(kind=kind, message=message) for kind, message in itertools.product(
        ['graph', 'schema', 'policy', 'id', 'conflict'], messages)]
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    if not same(expected, actual):
        raise SystemExit('Original/C++ bootstrap retry classification differs')
    provenance = json.loads((ROOT/'tests/parity/rust_field_layout/src/bootstrap_classifier_original.json').read_text())
    corpus = dict(baseline=provenance['runtime_commit'], producer=provenance,
                  note='85 observations of the exact extracted private original generated function. Error kind, substring/case behavior, primary/secondary keys and backend-shaped error text. No backend integrations or public initialization behavior are qualified by this corpus.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('bootstrap_classifier_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2)+'\n')
    print(f'Compared {len(inputs)} exact original/C++ bootstrap classifier observations')


if __name__ == '__main__':
    main()
