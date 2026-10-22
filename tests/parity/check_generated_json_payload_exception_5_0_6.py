#!/usr/bin/env python3
"""Verify the approved JSON payload exception, never an exact Rust parity claim."""
import argparse
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else ()
    [actual] = observe(args.cpp, [{'case': 'combined'}], emulator)
    payload = {'id': 1, 'version': 7, 'optional_count': None, 'quantity': 0,
               'active': False, 'height': 3.5, 'amount': '123.450',
               'day': '2024-02-29', 'recorded_at': 1700000000123,
               'payload': {'literal': '_comment'}, 'clock_text': '',
               '_name': 'derived', '_nil': None, '_count': 0}
    expected = {'json': payload, 'roundtrip': payload, 'null_loaded': True,
                'omitted_loaded': False, 'quantity': 0, 'active': False,
                'height': 3.5, 'amount': '123.450', 'day': '2024-02-29',
                'timestamp': 1700000000123, 'clock': '', 'dirty': None,
                'new': False, 'deleted': False, 'dynamic_pending': False}
    if not same(expected, actual):
        raise AssertionError(f'Approved JSON payload contract changed: {actual}')
    print('Approved C++ JSON payload exception: full scalar/read-only roundtrip passes')


if __name__ == '__main__':
    main()
