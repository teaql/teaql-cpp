#!/usr/bin/env python3
"""Record real Value From conversions, every extractor and pairwise equality."""
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
    values = [dict(kind='null')]
    values.extend(dict(kind='typed_null', type=dtype) for dtype in range(10))
    for width in (16, 32, 64):
        for number in (-(1 << (width - 1)), -1, 0, 1, (1 << (width - 1)) - 1):
            values.append(dict(kind=f'from_i{width}', value=str(number)))
        for number in (0, 1, (1 << (width - 1)) - 1, 1 << (width - 1), (1 << width) - 1):
            values.append(dict(kind=f'from_u{width}', value=str(number)))
    for width, patterns in ((32, ['00000000', '80000000', '00000001', '007fffff', '00800000',
                                  '3f800000', '3eaaaaab', '4b800001', '7f7fffff', '7f800000',
                                  'ff800000', '7fc00000', 'ffc00000']),
                            (64, ['0000000000000000', '8000000000000000', '0000000000000001',
                                  '000fffffffffffff', '0010000000000000', '3ff0000000000000',
                                  '3fd5555555555555', '4340000000000001', '7fefffffffffffff',
                                  '7ff0000000000000', 'fff0000000000000', '7ff8000000000000',
                                  'fff8000000000000'])):
        values.extend(dict(kind=f'from_f{width}_bits', value=bits) for bits in patterns)
    for kind in ('from_borrowed_text', 'from_owned_text'):
        for text in ('', 'false', '1.2300', '2024-02-29', '1969-12-31T23:59:59.999Z',
                     'long dynamic text\n雪\u0000' * 8, 'invalid'):
            values.append(dict(kind=kind, value=text))
    values.extend(dict(kind='from_bool', value=boolean) for boolean in (False, True))
    values.extend(dict(kind='from_decimal', value=number) for number in
                  ('0', '-0.00', '1.2300', '-1.2300', '79228162514264337593543950335',
                   '0.0000000000000000000000000001', '12345678901234567890.123456789'))
    values.extend(dict(kind='from_json', value=payload) for payload in
                  (None, False, True, 0, -1, 1.0, -0.0, '', [], {},
                   {'nested': [None, 1, 1.5, '雪']}, [1, {'v': 'long text' * 20}]))
    values.extend(dict(kind='from_date', value=date) for date in
                  ('1970-01-01', '1969-12-31', '2024-02-29'))
    values.extend(dict(kind='from_timestamp', value=str(number)) for number in
                  (-(1 << 63), -1, 0, 1, 1709164800000, (1 << 63) - 1))
    values.extend([dict(kind='list', value=[]), dict(kind='object', value={}),
                   dict(kind='list', value=[dict(kind='text', value='nested'),
                                            dict(kind='decimal', value='1.00')]),
                   dict(kind='object', value={'v': dict(kind='i64', value='1')})])
    cases = [dict(value=value, others=values) for value in values]
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='All Value variants and extractors, scalar From conversions, exact optional f64 bits, Clone and pairwise equality.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('value_api_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original Value API inputs and {len(cases) ** 2} equality pairs')


if __name__ == '__main__':
    main()
