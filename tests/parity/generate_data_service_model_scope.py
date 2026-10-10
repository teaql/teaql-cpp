#!/usr/bin/env python3
"""Record actual data-service model Debug/Clone derives from the pinned source."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    base = dict(mask=0, comment=None, trace=False, params=[], start='0', end='0',
                operation=0, count=0, affected='0', nested=False, rows=False, snapshot=False)
    cases = [base]
    for mask in [1 << bit for bit in range(8)] + [255]:
        cases.append(dict(base, mask=mask))
    for index, comment in enumerate([None, '', 'annotation\n\0雪😀', 'long-' + 'x' * 2048]):
        for traced in (False, True):
            cases.append(dict(base, comment=comment, trace=traced, rows=True,
                              snapshot=True, nested=True, count=17, affected=str((1 << 64) - 1),
                              params=[dict(kind='text', value='payload\0雪')], operation=index))
    pool = [dict(kind='null'), dict(kind='typed_null'), dict(kind='bool', value=True),
            dict(kind='i64', value=str(-(1 << 63))), dict(kind='u64', value=str((1 << 64) - 1)),
            dict(kind='f64', value='NaN'), dict(kind='f64', value='-0.0'), dict(kind='f64', value='inf'),
            dict(kind='decimal', value='-123.4500'), dict(kind='text', value='nested\n\0雪😀'),
            dict(kind='json', value={'key': [False, 1.25, None]}),
            dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='-1001'),
            dict(kind='list', value=[dict(kind='text', value='nested'), dict(kind='bool', value=False)]),
            dict(kind='object', value={'value': dict(kind='i64', value='-7')})]
    for index, value in enumerate(pool):
        cases.append(dict(base, mask=255, params=[value], operation=index % 7, count=(1 << 32) - 1,
                          rows=True, snapshot=True, nested=True, trace=True, comment='model'))
    for ticks in [-(1 << 63), (1 << 63) - 1, -1000000001, -1000000000, -1, 1, 1000000001]:
        cases.append(dict(base, start=str(ticks), end=str(-ticks if ticks != -(1 << 63) else 0)))
    return [dict(flags=case['mask'], text=case['comment'] or 'scope',
                 comment=case['comment'], count=case['count'],
                 start=case['start'], end=case['end'],
                 value=case['params'][0] if case['params'] else dict(kind='null'))
            for case in cases]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = inputs()
    results = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual original data-service Debug/Clone derives in nine formatter modes; no invented serde protocol. Time inputs cover the existing native signed-nanosecond domain; full source SystemTime seconds remain an explicit open gap.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, results)])
    payload = json.dumps(corpus, ensure_ascii=True, separators=(',', ':')).encode()
    output = Path(__file__).with_name('data_service_model_scope_cases.json.gz')
    with output.open('wb') as destination:
        with gzip.GzipFile(fileobj=destination, filename='', mode='wb', mtime=0) as compressed:
            compressed.write(payload)
    print(f'Recorded {len(cases)} original data-service model inputs')


if __name__ == '__main__':
    main()
