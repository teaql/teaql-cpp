#!/usr/bin/env python3
"""Verify every unpolled generated lookup-error operation cancels or closes with its Context."""
import argparse
import itertools
from compare_values import observe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    cases = [dict(ownership=True, mode=mode, cancel=cancel)
             for mode, cancel in itertools.product(
                 ['list', 'rows', 'first', 'one', 'count', 'exists', 'page', 'stream'],
                 [False, True])]
    assert observe(args.cpp, cases) == [True] * 16
    print('Verified 16 actual Context deletions for all eight unpolled/cancelled lookup failures')


if __name__ == '__main__':
    main()
