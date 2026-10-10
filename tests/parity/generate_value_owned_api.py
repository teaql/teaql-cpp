#!/usr/bin/env python3
"""Observe actual owned Value construction, storage transfer and complete projections."""
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
    def value(kind, data=None):
        return dict(kind=kind, value=data)
    pool = [value('null'), value('typed_null'), value('bool', False),
            value('i64', '-9223372036854775808'), value('u64', '18446744073709551615'),
            value('f64', 'NaN'), value('f64', '-0.0'), value('decimal', '123.450'),
            value('text', 'nested text\n\0雪'), value('date', '2024-02-29'),
            value('timestamp', '-1'), value('json', {'nested': [None, 1, 'secret']}),
            value('list', [value('text', 'nested')]), value('object', {'x': value('bool', True)})]
    cases = [dict(kind='text', text=text) for text in ('x' * 64, '雪\n\0' * 32, 'x' * 4096)]
    for count in (0, 1, 2, 20, 21, 64):
        for item in pool:
            cases.append(dict(kind='list', values=[item] * count))
            cases.append(dict(kind='object', values={f'key-{i:03d}': item for i in range(count)}))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original From<String>, List(Vec), object(BTreeMap); storage transfer and full values.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('value_owned_api_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} original owned Value construction observations')


if __name__ == '__main__':
    main()
