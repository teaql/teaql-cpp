#!/usr/bin/env python3
"""Record original remaining core business/search and Decimal helper diagnostics."""
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
    values = [None, False, True, 0, -1, 1.0, -0.0, '', 'private\n雪\u0000', [], {},
              {'items': [None, 1.0, '雪']}]
    texts = ['', 'private\n雪\u0000', 'quote"\\', '\u0085\u2028\u2029', '🧊', 'x' * 8192, 'root|%']
    def case(text, value, flags, number, digits, scale, words, date='2024-02-29'):
        cases.append(dict(text=text, value=value, flags=flags, number=number, digits=digits,
                          scale=scale, words=words, date=date))
    for value in values:
        for flags in (0, 1, 4, 15):
            case(texts[1], value, flags, (1 << 64) - 1, 255, 0xffffffff,
                 [0, 1 << 31, 0xffffffff])
    for text in texts:
        for flags in (0, 15):
            case(text, {}, flags, 0, 0, 0, [0, 0, 0])
    rng = random.Random(505)
    for _ in range(32):
        case(rng.choice(texts[:-1]), rng.choice(values), rng.randrange(32), rng.getrandbits(64),
             rng.choice([0, 6, 19, 255]), rng.getrandbits(32), [rng.getrandbits(32) for _ in range(3)],
             rng.choice(['0000-01-01', '2024-02-29', '-0001-12-31', '+10000-01-01']))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual original remaining eighteen derived core/dependency Debug types, inherited formatter modes, defined Clone equality and warning wire values. Capability presence is not full API/privacy acceptance.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    encoded = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('core_aux_debug_cases.json.gz').write_bytes(gzip.compress(encoded, mtime=0))
    print(f'Recorded {len(cases)} original auxiliary core diagnostic inputs')


if __name__ == '__main__':
    main()
