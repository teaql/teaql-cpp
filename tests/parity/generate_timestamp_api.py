#!/usr/bin/env python3
"""Record original Timestamp conversion, chrono fallback, Debug and wire acceptance."""
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
    cases = []
    values = {-(1 << 63), (1 << 63) - 1, -1, 0, 1}
    for base in (-8334601228800000, 8210266876799999, -2208988800000,
                 -86400000, -1000, 1000, 86400000, 951782400000,
                 1709164800000, 2147483647000, 4294967296000):
        values.update(base + offset for offset in (-1001, -1, 0, 1, 1001))
    cases.extend(dict(kind='signed', value=str(value)) for value in sorted(values))
    for value in (0, 1, 2000, (1 << 32) - 1, 1 << 32, (1 << 63) - 1,
                  1 << 63, (1 << 63) + 1, (1 << 64) - 2, (1 << 64) - 1):
        cases.append(dict(kind='unsigned', value=str(value)))
    wires = ['0', '-0', '  1000\n', '-1', '9223372036854775807', '-9223372036854775808',
             '9223372036854775808', '18446744073709551615', '-9223372036854775809',
             '18446744073709551616', '1.0', '-0.0', '1e0', '1e999', 'null', 'true',
             'false', '"1000"', '[]', '[1000]', '{}', '{"value":1000}', '', '1 2',
             '01', '+1', 'NaN', 'Infinity', '1.', '--1', '\n-2000\t']
    cases.extend(dict(kind='wire', wire=wire) for wire in wires)
    for value in (-(1 << 63), -86400001, -1001, -1, 0, 1,
                  951782400000, 8210266876799999, (1 << 63) - 1):
        cases.append(dict(kind='field', value='2000', replacement=str(value)))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual Timestamp public methods, derived Debug and transparent serde acceptance; error text is not observed.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('timestamp_api_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} original Timestamp API inputs')


if __name__ == '__main__':
    main()
