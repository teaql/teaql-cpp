#!/usr/bin/env python3
import argparse
import sys
from compare_values import main
if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--pointer-bytes', type=int, choices=(4, 8), default=8)
    args, remaining = parser.parse_known_args()
    sys.argv = [sys.argv[0], *remaining]
    corpus = 'query_keys_cases_32.json.gz' if args.pointer_bytes == 4 else 'query_keys_5_0_6_cases.json.gz'
    raise SystemExit(main(corpus, f'canonical and pagination query keys ({args.pointer_bytes * 8}-bit usize)'))
