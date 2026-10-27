#!/usr/bin/env python3
"""Recover the previous in-memory comparator and CMake registry byte for byte."""

import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INITIAL = ROOT / 'docs/parity/query_input_cli_source_milestone_5_0_6.json.gz'
PREVIOUS = ROOT / 'docs/parity/query_surface_cli_previous_input_delta_5_0_6.json.gz'


def remove_once(value, fragment):
    assert value.count(fragment) == 1, fragment
    return value.replace(fragment, '', 1)


def main():
    path = 'tests/parity/compare_inmemory.py'
    current = (ROOT / path).read_text()
    preceding = remove_once(current, 'from expression_cli_cases import require_cases\n')
    preceding = remove_once(preceding, '    require_cases(corpus, 6532)\n')
    preceding = remove_once(preceding,
        "    if len(actual) != len(inputs):\n"
        "        raise RuntimeError('Native observations must cover the complete input matrix')\n")
    initial = json.loads(gzip.decompress(INITIAL.read_bytes()))['input_hashes']
    assert hashlib.sha256(preceding.encode()).hexdigest() == initial['runtime:' + path]
    current = (ROOT / 'tests/CMakeLists.txt').read_text()
    marker = '\n\n# Bind the complete accepted in-memory source replay to its historical native CLI.\n'
    assert current.count(marker) == 1
    preceding = current.split(marker, 1)[0] + '\n'
    snapshot = json.loads(gzip.decompress(PREVIOUS.read_bytes()))
    assert hashlib.sha256(preceding.encode()).hexdigest() == snapshot[
        'changed_existing']['runtime:tests/CMakeLists.txt']['after']
    print('Recovered preceding in-memory comparison and CMake registry byte for byte')


if __name__ == '__main__':
    main()
