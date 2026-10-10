#!/usr/bin/env python3
"""Record original floating, primitive-integer and scaled-constructor calls."""
import argparse
import json
import random
import struct
import subprocess
from pathlib import Path

from compare_values import observe


def inputs():
    cases = []
    rng = random.Random(505)
    for width, mantissa, exponent in ((32, 23, 8), (64, 52, 11)):
        mask = (1 << width) - 1
        patterns = [0, 1, mask, 1 << (width - 1), (1 << mantissa) - 1,
                    1 << mantissa, ((1 << exponent) - 1) << mantissa]
        for power in (0, 1, 2, 10, 32, 64, 93, 94, 95, 96, 97, 127,
                      128, 129, 255, 1022, 1023, 1024, 1116, 1118, 1119, 1120, 2047):
            if power >= 1 << exponent:
                continue
            for fraction in (0, 1, (1 << mantissa) // 2, (1 << mantissa) - 1):
                patterns.append((power << mantissa) | fraction)
        for value in (0.1, 0.5, 1.0, 1.2, 1.25, 3.141592653589793,
                      1e-28, 1e-29, 1e28, 7.922816251426434e28):
            fmt = '>f' if width == 32 else '>d'
            integer = '>I' if width == 32 else '>Q'
            patterns.append(struct.unpack(integer, struct.pack(fmt, value))[0])
        patterns.extend(rng.getrandbits(width - 1) for _ in range(700))
        for bits in patterns:
            for sign in (0, 1 << (width - 1)):
                cases.append(dict(kind='numeric_float', width=width, bits=(bits | sign) & mask))
    numbers = [0, 1, (1 << 128) - 1, 1 << 127, (1 << 127) - 1]
    for power in (7, 8, 15, 16, 31, 32, 63, 64, 95, 96, 97, 126, 127):
        for offset in (-1, 0, 1):
            value = (1 << power) + offset
            numbers.extend((value, (-value) % (1 << 128)))
    numbers.extend(rng.getrandbits(128) for _ in range(100))
    for value in numbers:
        for scale in (0, 1, 2, 27, 28, 29, 255, (1 << 32) - 1):
            cases.append(dict(kind='numeric_wide', words=[(value >> (i * 32)) & 0xffffffff
                                                         for i in range(4)], scale=scale))
    for value in (-(1 << 63), -(1 << 31), -1, 0, 1, (1 << 31) - 1, (1 << 63) - 1):
        for scale in (0, 1, 28, 29, 255, (1 << 32) - 1):
            cases.append(dict(kind='numeric_i64', text=str(value), scale=scale))
    values = ['0', '-0', '0.000', '0.1', '-0.1', '1.25', '-1.25',
              '0.0000000000000000000000000001', '79228162514264337593543950335',
              '-79228162514264337593543950335']
    for power in (7, 8, 15, 16, 31, 32, 63, 64, 95):
        for offset in (-1, 0, 1):
            value = (1 << power) + offset
            values.extend((str(value), str(-value), str(value) + '.1'))
    for text in values:
        for scale in (0, 1, 2, 28, 29, 38, 39, 56):
            for negative in (False, True):
                cases.append(dict(kind='numeric_decimal', text=text, scale=scale, negative=negative))
    return list({json.dumps(case, sort_keys=True): case for case in cases}.values())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=lock['commit'],
                  dependency='rust_decimal 1.42.1; tests/parity/rust-values/Cargo.lock',
                  note='Original numeric conversions; words are little-significance-first, signed words use two\'s complement.',
                  cases=[dict(input=case, expected=output) for case, output in zip(cases, outputs)])
    Path(__file__).with_name('decimal_numeric_cases.json').write_text(
        json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} Decimal numeric observations')


if __name__ == '__main__':
    main()
