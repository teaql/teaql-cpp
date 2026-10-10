#!/usr/bin/env python3
"""Record original query/Expr/Trace derived formatting, not a copied formatter."""
import argparse
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
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    values = [dict(kind='null'), dict(kind='bool', value=False), dict(kind='bool', value=True),
              dict(kind='i64', value=str(-(1 << 63))), dict(kind='u64', value=str((1 << 64) - 1)),
              dict(kind='f64', value='-0'), dict(kind='f64', value='NaN'),
              dict(kind='decimal', value='123456789.123400'),
              dict(kind='text', value='private\n雪\u0000' * 8),
              dict(kind='json', value={'items': [None, True, 1.0, '雪']}),
              dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='-1'),
              dict(kind='list', value=[dict(kind='i64', value='1'), dict(kind='text', value='nested')]),
              dict(kind='object', value={'private': dict(kind='decimal', value='1.00')}),
              dict(kind='typed_null')]
    cases = []
    for index, value in enumerate(values):
        for flags in range(8):
            cases.append(dict(flags=flags, value=value, number=str((0, 17, (1 << 64) - 1)[(index + flags) % 3]),
                              op=index % 12, function=index % 14, aggregate=index % 12,
                              trace=index % 9, enums=not cases))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original derived Debug for every query helper, all Expr variants and nested Trace, nine formatter modes.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('query_debug_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original query Debug inputs')


if __name__ == '__main__':
    main()
