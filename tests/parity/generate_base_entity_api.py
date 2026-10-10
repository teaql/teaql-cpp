#!/usr/bin/env python3
"""Observe unchanged BaseEntityData parsing, exact errors, map conversion and Clone."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    def value(kind, data=None):
        return dict(kind=kind, value=data)
    pool = [None, value('null'), value('bool', True), value('bool', False)]
    pool += [value('i64', str(n)) for n in (-9223372036854775808, -1, 0, 1, 9223372036854775807)]
    pool += [value('u64', str(n)) for n in (0, 1, 9223372036854775807, 18446744073709551615)]
    pool += [value('f64', n) for n in ('0.0', '-0.0', 'NaN', 'inf', '-inf')]
    pool += [value('decimal', n) for n in ('0', '-1', '123.450', '79228162514264337593543950335')]
    pool += [value('text', 'text\n\0雪'), value('date', '2024-02-29'), value('timestamp', '-1'),
             value('json', {'nested': [None, True, 'secret']}),
             value('list', [value('text', 'nested'), value('i64', '-1')]),
             value('object', {'nested': value('bool', True)})]
    pool += [value('typed_null_type', i) for i in range(10)]
    cases = []
    for identity in pool:
        for version in pool:
            fields = {'extra': value('object', {'nested': value('text', 'copied payload')})}
            if identity is not None:
                fields['id'] = identity
            if version is not None:
                fields['version'] = version
            cases.append(dict(values=fields))
    for dynamic in pool[1:]:
        cases.append(dict(values={'id': value('u64', '7'), 'version': value('i64', '-3'),
                                  'payload': dynamic, '': value('text', 'empty key')}))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original BaseEntityData methods: Value variants, complete errors, precedence and Clone.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('base_entity_api_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} original BaseEntityData API observations')


if __name__ == '__main__':
    main()
