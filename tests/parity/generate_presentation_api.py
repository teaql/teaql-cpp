#!/usr/bin/env python3
"""Record original presentation traits, field defaults and JSON acceptance/roundtrips."""
import argparse
import copy
import gzip
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
    samples = {
        'style': {'backgroundColor': '', 'color': 'private\n雪', 'classNames': 'class'},
        'action': {'key': '', 'name': 'name', 'level': 'level', 'execute': 'execute', 'target': 'target',
                   'component': 'component', 'warningMessage': '', 'roleForList': 'role', 'requestURL': '/URL'},
        'response': {'data': [None, True, 1.0, {'nested': ['雪']}], 'resultCode': -2147483648,
                     'status': '', 'message': '', 'recordCount': (1 << 64) - 1, 'version': '', 'facets': {'f': [1, 2]}},
        'block': {'page': 'page', 'top': -2147483648, 'bottom': 2147483647, 'left': 1, 'right': 3,
                  'styleReferBlock': None, 'value': {'nested': [None, 1.0, '雪']}, 'properties': {'p': True}},
        'context': {'page': 'page', 'startX': -2, 'x': -1, 'y': -3},
        'page': {'name': 'page', 'blocks': []}, 'workbook': {'pages': []},
    }
    samples['page']['blocks'] = [copy.deepcopy(samples['block'])]
    samples['workbook']['pages'] = [copy.deepcopy(samples['page'])]
    optional = {'style': set(samples['style']), 'action': set(samples['action']),
                'response': {'status', 'message'}, 'block': {'styleReferBlock'},
                'context': set(), 'page': set(), 'workbook': set()}
    cases = []
    for kind, sample in samples.items():
        def add(value):
            cases.append(dict(kind=kind, value=value))
        add(copy.deepcopy(sample))
        add({})
        unknown = copy.deepcopy(sample)
        unknown['unknownField'] = {'private': 1}
        add(unknown)
        for field in sample:
            missing = copy.deepcopy(sample)
            del missing[field]
            add(missing)
            for wrong in (None, False, 0, 1.0, '', [], {}):
                changed = copy.deepcopy(sample)
                changed[field] = wrong
                add(changed)
        for wrong_root in (None, False, 0, 1.0, ''):
            add(wrong_root)
        sequence = list(copy.deepcopy(sample).values())
        for length in range(len(sequence) + 2):
            add(sequence[:length] if length <= len(sequence) else sequence + [None])
        if optional[kind]:
            changed = copy.deepcopy(sample)
            for name in optional[kind]:
                changed[name] = None
            add(changed)
    for field in ('top', 'bottom', 'left', 'right'):
        for value in (-2147483649, 2147483648):
            changed = copy.deepcopy(samples['block'])
            changed[field] = value
            cases.append(dict(kind='block', value=changed))
    for value in (-1, 1 << 64, 1.0):
        changed = copy.deepcopy(samples['response'])
        changed['recordCount'] = value
        cases.append(dict(kind='response', value=changed))
    nested = copy.deepcopy(samples['block'])
    nested['styleReferBlock'] = copy.deepcopy(samples['block'])
    cases.append(dict(kind='block', value=nested))
    for payload in (None, False, True, 0, -1, 1.0, -0.0, '', 'private\n雪\u0000', [], {}, {'items': [None, 1.0, '雪']}):
        for mode in range(3):
            for present in (False, True):
                cases.append(dict(kind='bindings', value=payload, mode=mode, present=present))
        cases.append(dict(kind='ranges', value=payload))
        for flags in range(4):
            cases.append(dict(kind='construct', value=payload, flags=flags))
    whitespace = list(range(9,14)) + [0x20,0x85,0xa0,0x1680] + list(range(0x2000,0x200b)) + [0x2028,0x2029,0x202f,0x205f,0x3000]
    boundaries = [0,0x1c,0x1d,0x1e,0x1f,0x180e,0x200b,0x2060,0xfeff]
    for codepoint in whitespace + boundaries:
        char = chr(codepoint)
        for text in [char, char+'core'+char, char+' core '+char, 'left'+char+'right']:
            cases.append(dict(kind='trim', value=text))
    for value in ['', ' \t\n雪\u3000', '\u3000'+'x'*8192+'\u00a0', None, True, 1, 1.5, [], {}]:
        cases.append(dict(kind='trim', value=value))
    for value in [None, {'nested':['雪',1,None]}, 'x'*4096]:
        for mode in ['move_iterator','value_iterator']:
            cases.append(dict(kind='owning_ranges', value=value, mode=mode))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual original seven presentation model traits, JSON derives, missing/default/null/type/sequence/unknown fields and navigation/builders; external serde error text is not observed.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('presentation_api_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original presentation model API inputs')


if __name__ == '__main__':
    main()
