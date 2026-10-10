#!/usr/bin/env python3
"""Record actual derived DataType Debug, equality and containing TypedNull Debug."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    cases = [dict(type=i) for i in range(10)]
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original derived DataType Debug and equality; nine modes per type/TypedNull.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('datatype_debug_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print('Recorded 10 DataTypes, 180 Debug outputs and 100 equality results')


if __name__ == '__main__':
    main()
