#!/usr/bin/env python3
"""Record original heap-free Decimal array_string and extended-scale boundaries."""
import argparse
import gzip
import json
import random
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = []
    coefficients = ((0, 0, 0), (1, 0, 0), (1290, 0, 0), (0xffffffff, 0, 0),
                    (0, 1, 0), (0, 0, 1), (0xffffffff, 0xffffffff, 0xffffffff))
    for words in coefficients:
        for negative in (False, True):
            for scale in (0, 1, 28):
                for rescale in (None, 0, 28, 29, 30, 31, 32, 33, 255, 0xffffffff):
                    cases.append(dict(words=list(words), negative=negative, scale=scale,
                                      rescale=rescale))
    rng = random.Random(505)
    for _ in range(64):
        cases.append(dict(words=[rng.getrandbits(32) for _ in range(3)],
                          negative=bool(rng.getrandbits(1)), scale=rng.randrange(29),
                          rescale=rng.choice((None, 28, 29, 30, 31, 32, 33))))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original Decimal::array_string with inline storage, unsigned text and exact original fixed-buffer panic boundaries; no source formatter is copied into the oracle.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    encoded = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('decimal_inline_cases.json.gz').write_bytes(
        gzip.compress(encoded, mtime=0))
    print(f'Recorded {len(cases)} original Decimal inline-string inputs')


if __name__ == '__main__':
    main()
