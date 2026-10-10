#!/usr/bin/env python3
"""Record unchanged original SQL models, binding scopes and error contracts."""
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
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    payloads = [dict(kind='null'), dict(kind='typed_null'), dict(kind='bool', value=True),
                dict(kind='i64', value='-9223372036854775808'),
                dict(kind='u64', value='18446744073709551615'),
                dict(kind='f64', value='-0.0'), dict(kind='decimal', value='12.3400'),
                dict(kind='text', value="O'Reilly\n雪\0"),
                dict(kind='json', value={'password': 'private-json', 'a': [None, False, 1]}),
                dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='-1'),
                dict(kind='object', value={'nested': dict(kind='text', value='retained')}),
                dict(kind='list', value=[dict(kind='i64', value='-1'), dict(kind='text', value='list')])]
    cases = []
    for index in range(40):
        params = [dict(v) for v in payloads] if index % 4 else []
        if index % 3 == 0:
            for position, value in enumerate(params):
                value['field'] = ['plain', 'masked', 'token', 'missing'][position % 4]
        sql = 'SELECT ' + ', '.join('?' for _ in params) if params else 'SELECT 1'
        if index % 7 == 0:
            sql += ' /* ? quoted */ -- ?\n'
        if index % 11 == 0:
            sql += ', :unsupported'
        cases.append(dict(text=['', 'private\n雪\0', 'quote"\\', 'x' * 4096][index % 4],
                          scope=[None, 0, 1, 2, 3][index % 5], params=params,
                          untrusted=bool(index % 2), sql=sql,
                          comment=[None, '', 'why */ nested /* */', '🧊\n'][index % 4],
                          version=['-9223372036854775808', '-1', '0', '9223372036854775807'][index % 4],
                          type=index % 10, bytes=[0, 63, 64, 0xffffffff][index % 4],
                          outcome=[None, 0, 1, 2][index % 4],
                          mode=[None, '', 'safe'][index % 3], omission=[None, '', 'reason'][index % 3],
                          masked=[False, True] if index % 2 else []))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual original SQL models; binding scope uses the complete unchanged pinned bindings.rs. No copied behavior algorithms.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    encoded = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('sql_model_api_cases.json.gz').write_bytes(gzip.compress(encoded, mtime=0))
    print(f'Recorded {len(cases)} original SQL model observations')


if __name__ == '__main__':
    main()
