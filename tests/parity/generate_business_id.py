#!/usr/bin/env python3
"""Refresh pinned Business ID observations and every fieldwise equality case."""
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
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    path = Path(__file__).with_name('business_id_cases.json')
    data = json.loads(path.read_text())
    commit = json.loads((root / 'upstream.lock.json').read_text())['commit']
    if data['baseline'] != commit:
        raise RuntimeError('Business ID seed inputs belong to a different baseline')
    # Retain the established constructor/range/key/error inputs. Equality inputs
    # are regenerated rather than duplicated on subsequent recorder runs.
    inputs = [case['input'] for case in data['cases']
              if case['input']['kind'] != 'equality']
    for model, count in [('definition', 7), ('scope', 4), ('request', 10),
                         ('plan', 10), ('allocation', 5), ('value', 1), ('error', 2)]:
        inputs.extend(dict(kind='equality', model=model, field=field)
                      for field in range(-1, count))
    inputs.extend(dict(kind='equality', model='code', field=-1, left=left, right=right)
                  for left in range(7) for right in range(7))
    outputs = observe(args.rust, inputs)
    data['note'] = (
        'Actual pinned Rust core Business ID constructors, full-width sequence bounds, '
        'scope canonicalization, allocation ranges, Display/default empty Error source '
        'chain and per-field derived equality including all 49 error-code pairs. JSON '
        'is test observation format; these source models have no serde protocol.')
    data['cases'] = [dict(input=inp, expected=out) for inp, out in zip(inputs, outputs)]
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n')
    print(f'Recorded {len(inputs)} actual Rust Business ID inputs')


if __name__ == '__main__':
    main()
