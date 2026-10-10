#!/usr/bin/env python3
"""Record actual pinned data-service model/default and diagnostic calls."""
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
    values = [dict(kind='null'), dict(kind='typed_null'), dict(kind='bool', value=True),
              dict(kind='i64', value='-9223372036854775808'),
              dict(kind='u64', value='18446744073709551615'), dict(kind='f64', value='-0.0'),
              dict(kind='decimal', value='12.3400'), dict(kind='text', value='private\n雪\0'),
              dict(kind='json', value={'nested': [None, True, 'data']}),
              dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='-1'),
              dict(kind='list', value=[dict(kind='text', value='nested')]),
              dict(kind='object', value={'item': dict(kind='text', value='nested')})]
    times = ['0', '-1', '1', '-1000000001', '1000000001',
             '-9223372036854775808', '9223372036854775807']
    cases = []
    for index in range(40):
        flags = [0, 255, *[1 << bit for bit in range(8)]][index % 10] if index < 10 else (index * 37) % 256
        cases.append(dict(flags=flags, text=['', 'private\n雪\0', 'x' * 1024][index % 3],
                          comment=[None, '', 'request comment'][index % 3],
                          count=[0, 1, 0xffffffff][index % 3],
                          start=times[index % len(times)], end=times[(index + 1) % len(times)],
                          value=values[index % len(values)]))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual original data-service models, derives, Clone and unrecorded-query construction. Linux SystemTime diagnostics; the nanosecond input domain is explicit, not a claim of the complete Rust SystemTime range.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    encoded = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('data_service_model_cases.json.gz').write_bytes(gzip.compress(encoded, mtime=0))
    print(f'Recorded {len(cases)} original data-service model inputs')


if __name__ == '__main__':
    main()
