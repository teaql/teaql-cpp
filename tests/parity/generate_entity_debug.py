#!/usr/bin/env python3
"""Record unchanged derived EntityError/BaseEntityData Debug with formatter flags."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    def v(kind, data=None):
        return dict(kind=kind, value=data)
    values = [v('null'), v('bool', True), v('i64', '-9223372036854775808'),
              v('u64', '18446744073709551615'), v('f64', 'NaN'), v('f64', '-0.0'),
              v('f64', 'inf'), v('decimal', '123.450'), v('text', '雪\n\0secret'),
              v('date', '2024-02-29'), v('timestamp', '-1'),
              v('json', {'nested': [1, None, 'secret']}),
              v('list', [v('text', 'nested'), v('i64', '-1')]),
              v('object', {'nested': v('bool', True)})]
    values += [v('typed_null_type', i) for i in range(10)]
    cases = []
    for identity in (0, 1, 18446744073709551615):
        for version in (-9223372036854775808, -1, 0, 9223372036854775807):
            for value in values:
                cases.append(dict(id=str(identity), version=str(version),
                                  dynamic={'': value, '雪': v('text', 'nested\n\0text')},
                                  entity='Edge\n雪', message='raw "secret"\0\t'))
    cases.append(dict(id='0', version='0', dynamic={}, entity='', message=''))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original derived Debug, nine formatter modes per EntityError/BaseEntityData.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('entity_debug_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} inputs and {len(cases) * 18} original Debug outputs')


if __name__ == '__main__':
    main()
