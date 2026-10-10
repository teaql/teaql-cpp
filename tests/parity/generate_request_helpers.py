#!/usr/bin/env python3
"""Record actual original public request helper outcomes, including exact panic messages."""
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
    vectors = [[], *[[value] for value in values],
               [dict(kind='i64', value='1'), dict(kind='i64', value='2')],
               [dict(kind='text', value=' a '), dict(kind='text', value='b')],
               [dict(kind='u64', value=str(i)) for i in range(20)],
               [dict(kind='u64', value=str(i)) for i in range(21)]]
    cases = [dict(kind='operators', operator=op, values=vector, index=index)
             for op in range(18) for vector in vectors for index in (0, len(vector))]
    dynamic = [None, False, True, 0, -1, (1 << 63) - 1, 1 << 63, (1 << 64) - 1,
               -(1 << 63), 1.0, -0.0, 0.125, '', '  雪\n ', '__IS_NULL__', '__is_not_null__',
               ' __is_null__ ', [], [1], [1, 2], [1, 2, 3], ['a', 'b'], [{"id": 1}, {"id": 2}],
               [None, True], {}, {'id': 0}, {'id': '  12  '}, {'id': {'id': [1, 2]}},
               {'other': 7}, list(range(20)), list(range(21)), ['a'] * 21]
    cases.extend(dict(kind='dynamic', json=value) for value in dynamic)
    cases.extend(dict(kind='filters', value=value, shape=shape) for value in values for shape in range(7))
    cases.extend(dict(kind='facets', value=value) for value in values)
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual request helpers; all operators, exact panic text, dynamic JSON, filter removal/NaN and facet replacement.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('request_helpers_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original request helper inputs')


if __name__ == '__main__':
    main()
