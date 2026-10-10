#!/usr/bin/env python3
"""Record original serde JSON and Display/Debug/LowerExp/UpperExp calls."""
import argparse
import json
import random
import subprocess
from pathlib import Path

from compare_values import observe


def inputs():
    cases = []
    values = ['0', '0.00', '1', '1.00', '0.1', '1.25', '1.235', '12.3000',
              '10000', '12345678901234567890.123456789',
              '0.0000000000000000000000000001', '79228162514264337593543950335']
    for text in values:
        for negative in (False, True):
            for scale in (0, 2, 28, 29, 30, 31, 32, 38, 56):
                for precision in (None, 0, 1, 2, 6, 28, 29, 30, 32, 40):
                    cases.append(dict(kind='decimal_format', text=text, negative=negative,
                                      scale=scale, precision=precision, width=40))
    literals = [None, True, False, [], [1], {}, {'a': 1}, '', '.', '_', '1.2.3',
                '0', '-0', '-0.00', '12.3400', '1_2.3', '.25', '+001.0',
                '1e2', '2.5e-28', '1e-29', '1e30', '1\0', 'é', '🙂', '\u0085',
                '79228162514264337593543950335', '79228162514264337593543950336']
    cases.extend(dict(kind='decimal_json', json=json.dumps(value, ensure_ascii=True))
                 for value in literals)
    raw = ['0', '-0', '0.0', '-0.0', '1.00', '12.345', '0.1', '1e-28', '1e-29',
           '1e28', '1e29', '-1e29', '18446744073709551615', '18446744073709551616',
           '-9223372036854775808', '-9223372036854775809',
           '79228162514264337593543950335', '0.0000000000000000000000000001',
           '0.9999999999999999999999999999', '12345678901234567890.123456789']
    rng = random.Random(505)
    for _ in range(400):
        digits = ''.join(rng.choice('0123456789') for _ in range(rng.randint(1, 30)))
        position = rng.randrange(1, len(digits) + 1)
        text = str(int(digits[:position]))
        if position < len(digits):
            text += '.' + digits[position:]
        text += 'e' + str(rng.randint(-35, 35))
        text = rng.choice(('', '-')) + text
        raw.append(text)
        cases.append(dict(kind='decimal_json', json=json.dumps(text)))
    cases.extend(dict(kind='decimal_json', json=value) for value in raw)
    return list({json.dumps(case, sort_keys=True): case for case in cases}.values())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    cases = inputs()
    results = observe(args.rust, cases)
    corpus = dict(baseline=lock['commit'],
                  dependency='rust_decimal 1.42.1, serde default/std; tests/parity/rust_values/Cargo.lock',
                  note='Original serde_json::from_value/to_value and formatting macros; no optional serde modes.',
                  cases=[dict(input=case, expected=output) for case, output in zip(cases, results)])
    Path(__file__).with_name('decimal_wire_format_cases.json').write_text(
        json.dumps(corpus, indent=2, ensure_ascii=True) + '\n')
    print(f'Recorded {len(cases)} Decimal JSON/format observations')


if __name__ == '__main__':
    main()
