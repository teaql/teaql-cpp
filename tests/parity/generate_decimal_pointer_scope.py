#!/usr/bin/env python3
"""Record actual original Decimal pointer conversions on 32-bit and 64-bit Rust."""
import argparse
import gzip
import itertools
import json
import subprocess
from pathlib import Path

from compare_values import observe, same


def inputs():
    values = ['0', '0.1', '1.25', '127', '128', '65535',
              '2147483647', '2147483648', '2147483648.9', '4294967295',
              '4294967296', '4294967296.1', '9223372036854775807',
              '9223372036854775808', '18446744073709551615',
              '18446744073709551616', '79228162514264337593543950335']
    signed = [str(value) for value in [-(1 << 63), -(1 << 31) - 1, -(1 << 31), -1,
                                      0, (1 << 31) - 1, 1 << 31, (1 << 63) - 1, 1 << 63]]
    unsigned = [str(value) for value in [0, 1, (1 << 31) - 1, 1 << 31,
                                        (1 << 32) - 1, 1 << 32, (1 << 64) - 1, 1 << 64]]
    for index, (value, negative, scale) in enumerate(itertools.product(values, (False, True),
                                                                     (0, 1, 2, 28, 29, 38, 56))):
        yield dict(kind='numeric_pointer', text=value, negative=negative, scale=scale,
                   signed=signed[index % len(signed)], unsigned=unsigned[index % len(unsigned)])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust64', required=True)
    parser.add_argument('--rust32', required=True)
    parser.add_argument('--rust32-emulator', default='qemu-arm')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    values = list(inputs())
    profiles = {}
    for bits, executable, emulator in [
        (64, args.rust64, []), (32, args.rust32, args.rust32_emulator.split(';')),
    ]:
        actual_bits = int(subprocess.check_output(emulator + [executable, '--pointer-bits'], text=True))
        if actual_bits != bits:
            raise RuntimeError(f'Expected real {bits}-bit oracle, observed {actual_bits}')
        output = observe(executable, values, emulator)
        if not same(output, observe(executable, values, emulator)):
            raise RuntimeError(f'{bits}-bit original observations changed between executions')
        if any(value['bits'] != bits for value in output):
            raise RuntimeError('Oracle output uses the wrong pointer width')
        profiles[str(bits)] = output
    corpus = dict(
        baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
        dependency='rust_decimal 1.42.1; rust-decimal-pointer/Cargo.lock',
        note='Actual original ToPrimitive, TryFrom, FromPrimitive and From at native 64-bit and '
             'ARMv7 musl 32-bit pointer widths. ARM source execution is not Rust MIPS execution.',
        cases=[dict(input=value, expected={bits: output[index] for bits, output in profiles.items()})
               for index, value in enumerate(values)],
    )
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('decimal_pointer_scope_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(values)} exact pointer conversions for each actual Rust word width')


if __name__ == '__main__':
    main()
