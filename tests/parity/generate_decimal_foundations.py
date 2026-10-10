#!/usr/bin/env python3
"""Record original scientific, Signed, Hash, unpack, Sum and Product calls."""
import argparse
import itertools
import json
import random
import subprocess
from pathlib import Path

from compare_values import observe


def inputs():
    cases = [dict(kind='constants')]
    bases = ['', '0', '-0', '0.000', '-0.000', '1', '-1', '1.2', '2.5', '-2.5',
             '2.710505431213761', '0.0000000000000000000000000001',
             '1.2345678901234567890123456789', '79228162514264337593543950335',
             '7922816251426433759354395033.5', '7.9228162514264337593543950335',
             '123.45000', '1_2.3', 'x', '.', '_1', '1.2.3']
    exponents = ['', '0', '-0', '+0', '1', '-1', '+1', '-+1', '--1', '+-1',
                 '18', '19', '20', '27', '28', '29', '-27', '-28', '-29',
                 '4294967295', '4294967296', '-4294967295', '-4294967296',
                 '+0002', '1e1', '1E1', '_1', '1_0', ' 1', '1 ', 'x', '\0']
    for base in bases:
        cases.append(dict(kind='scientific', text=base))
        for exponent in exponents:
            for separator in ('e', 'E'):
                cases.append(dict(kind='scientific', text=base + separator + exponent))
    rng = random.Random(505)
    for _ in range(600):
        digits = ''.join(rng.choice('0123456789') for _ in range(rng.randint(1, 35)))
        position = rng.randrange(len(digits) + 1)
        base = digits[:position] + '.' + digits[position:]
        text = rng.choice(('', '-')) + base + rng.choice(('e', 'E')) + str(rng.randint(-32, 32))
        cases.append(dict(kind='scientific', text=text))
    values = ['0', '0.000', '1', '1.0000', '1.25', '2.5', '123.45000',
              '0.0000000000000000000000000001', '79228162514264337593543950335']
    others = ['0', '1', '-1', '2.50', '-79228162514264337593543950335']
    for value in values:
        for other in others:
            for negative in (False, True):
                for scale in (0, 1, 2, 28, 29, 38, 56):
                    for positive in (False, True):
                        cases.append(dict(kind='traits', text=value, other=other,
                                          negative=negative, scale=scale, positive=positive))
    folds = [[], ['0.000'], ['1.00'], ['1.2', '3.4'], ['-0.000', '0.000'],
             ['79228162514264337593543950335', '1', '-1'],
             ['-79228162514264337593543950335', '-1', '1'],
             ['79228162514264337593543950335', '0.1', '10'],
             ['0.0000000000000000000000000001', '0.0000000000000000000000000001']]
    pool = values + ['-1', '-1.00', '-3.45', '10', '100', '0.1']
    folds.extend([rng.choice(pool) for _ in range(rng.randint(0, 8))] for _ in range(250))
    cases.extend(dict(kind='fold', values=values) for values in folds)
    for value, negative, scale in itertools.product(
            ['0', '1', '2', '3', '1.20', '10', '0.1', '1e-28',
             '79228162514264337593543950335'], (False, True), (0, 1, 28, 29, 56)):
        cases.append(dict(kind='reciprocal', text=value, negative=negative, scale=scale))
    for value, negative, scale in itertools.product(
            ['0', '1', '2', '3', '1.20', '10', '0.1', '1e-28',
             '79228162514264337593543950335'], (False, True), (0, 1, 28, 29, 56)):
        cases.append(dict(kind='identities', text=value, negative=negative, scale=scale))
    shapes = [('0', False, 0), ('0', True, 3), ('1', False, 0), ('1', False, 2),
              ('1', True, 0), ('3', False, 1),
              ('79228162514264337593543950335', False, 0),
              ('79228162514264337593543950335', True, 0),
              ('1e-28', False, 28), ('1', False, 29), ('3', True, 56)]
    bounds = [('0', '1'), ('1.00', '1'), ('-1', '1'), ('2', '1'),
              ('-79228162514264337593543950335', '79228162514264337593543950335')]
    def scalar(shape):
        text, negative, scale = shape
        return dict(text=text, negative=negative, scale=scale)
    for left, right, (minimum, maximum) in itertools.product(shapes, shapes, bounds):
        low = dict(text=minimum, negative=minimum.startswith('-'), scale=2 if minimum == '1.00' else 0)
        high = dict(text=maximum, negative=maximum.startswith('-'), scale=0)
        cases.append(dict(kind='ord_defaults', left=scalar(left), right=scalar(right),
                          minimum=low, maximum=high))
    return list({json.dumps(case, sort_keys=True): case for case in cases}.values())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=lock['commit'],
                  dependency='rust_decimal 1.42.1; tests/parity/rust_values/Cargo.lock',
                  note='Original scientific constructors, Signed, unpack, four u32 Hash writes, Sum and Product; seed 505.',
                  cases=[dict(input=case, expected=output) for case, output in zip(cases, outputs)])
    Path(__file__).with_name('decimal_foundation_cases.json').write_text(
        json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} Decimal foundation observations')


if __name__ == '__main__':
    main()
