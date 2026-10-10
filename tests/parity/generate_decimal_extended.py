#!/usr/bin/env python3
"""Observe original Decimal arithmetic after explicit extended rescaling."""
import argparse
import gzip
import itertools
import json
import random
import subprocess
from pathlib import Path

from compare_values import observe


def inputs():
    values = ['0', '1', '2.5', '0.0000000000000000000000000001',
              '1.2345678901234567890123456789', '79228162514264337593543950335']
    scales = [0, 28, 29, 38, 56, 255, 4294967295]
    cases = []
    for a, b, a_scale, b_scale, a_negative, b_negative in itertools.product(
            values, values, scales, scales, (False, True), (False, True)):
        cases.append(dict(kind='extended_binary', a=a, b=b, a_scale=a_scale,
                          b_scale=b_scale, a_negative=a_negative, b_negative=b_negative))
    rng = random.Random(505)
    for _ in range(1000):
        pair = []
        for _ in range(2):
            coefficient = rng.randrange(1 << 96)
            scale = rng.randrange(29)
            digits = str(coefficient).zfill(scale + 1)
            pair.append(digits if scale == 0 else digits[:-scale] + '.' + digits[-scale:])
        cases.append(dict(kind='extended_binary', a=pair[0], b=pair[1],
                          a_scale=rng.randrange(57), b_scale=rng.randrange(57),
                          a_negative=bool(rng.randrange(2)), b_negative=bool(rng.randrange(2))))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  dependency='rust_decimal 1.42.1; tests/parity/rust_values/Cargo.lock',
                  note='Direct checked/saturating arithmetic and ordering after rescale; seed 505.',
                  cases=[dict(input=case, expected=output) for case, output in zip(cases, outputs)])
    # Deterministic compressed observations retain every input and result field.
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('decimal_extended_cases.json.gz').write_bytes(
        gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original extended Decimal observations')


if __name__ == '__main__':
    main()
