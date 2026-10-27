#!/usr/bin/env python3
"""Reject changed original-test evidence through the installed relation checker."""

import argparse
import contextlib
import gzip
import importlib.util
import io
import json
from pathlib import Path
import sys
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
CHECKER = ROOT / 'tests/parity/check_relation_probe_boundaries_5_0_6.py'
ORIGINAL = ROOT / 'docs/parity/relation_source_original_tests_5_0_6.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()

    spec = importlib.util.spec_from_file_location('relation_source_boundary_checker', CHECKER)
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    assert checker.digest(ORIGINAL) == checker.ORIGINAL_RECEIPT_SHA256

    command = [str(CHECKER), '--cpp', str(args.cpp.resolve())]
    with patch.object(sys, 'argv', command), contextlib.redirect_stdout(io.StringIO()):
        checker.main()

    altered = json.loads(gzip.decompress(ORIGINAL.read_bytes()))
    assert len(altered['expected_tests']) == 3
    altered['expected_tests'][0] = 'fabricated::test::not_in_original_source'
    corrupted = gzip.compress((json.dumps(altered) + '\n').encode(), mtime=0)
    assert corrupted != ORIGINAL.read_bytes()
    ordinary_read = Path.read_bytes

    def substitute(path):
        if Path(path).resolve() == ORIGINAL:
            return corrupted
        return ordinary_read(path)

    with patch.object(Path, 'read_bytes', substitute), patch.object(sys, 'argv', command):
        try:
            checker.main()
        except AssertionError as error:
            assert str(error) == 'original relation test receipt byte identity changed'
        else:
            raise AssertionError('altered original relation test receipt was accepted')

    print('Original relation receipt: unchanged checker passes; changed test identity rejects')


if __name__ == '__main__':
    main()
