#!/usr/bin/env python3
"""Observe unchanged entity-registration macro and overridden trait defaults."""
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
    inputs = [dict(scenario=s, limit=n) for s in range(6) for n in range(7)]
    outputs = observe(args.rust, inputs)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original register_entities macro: overridden/default hooks and store failure order.',
                  cases=[dict(input=i, expected=o) for i, o in zip(inputs, outputs)])
    Path(__file__).with_name('entity_registration_api_cases.json').write_text(
        json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(inputs)} original entity registration observations')


if __name__ == '__main__':
    main()
