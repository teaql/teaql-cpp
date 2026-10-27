#!/usr/bin/env python3
"""Recover preceding typed-Q comparators and native test registration exactly."""

import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / 'docs/parity/query_input_cli_source_milestone_5_0_6.json.gz'
PREVIOUS = ROOT / 'docs/parity/runtime_error_cli_previous_input_delta_5_0_6.json.gz'
NAMES = ('query_alias', 'relation_identity', 'query_predicate_execution')


def remove_once(data, fragment):
    assert data.count(fragment) == 1, fragment
    return data.replace(fragment, '', 1)


def main():
    prior = json.loads(gzip.decompress(INPUTS.read_bytes()))['input_hashes']
    for name in NAMES:
        path = 'tests/parity/compare_' + name + '_5_0_6.py'
        current = (ROOT / path).read_text()
        previous = remove_once(current, 'from expression_cli_cases import require_cases\n')
        if name == 'query_predicate_execution':
            previous = remove_once(previous, '        require_cases(corpus, len(cases), cases)\n')
            previous = remove_once(previous,
                "    if len(actual) != len(cases):\n"
                "        raise RuntimeError('Native observations must cover the complete input matrix')\n")
        elif name == 'query_alias':
            previous = remove_once(previous, '        require_cases(corpus, len(cases), cases)\n')
            previous = remove_once(previous,
                "    if len(actual) != len(cases):\n"
                "        raise RuntimeError('Native observations must cover the complete input matrix')\n")
        else:
            previous = remove_once(previous, '        require_cases(corpus, len(cases), cases)\n')
            old = '    actual=observe(args.cpp,cases);failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]\n'
            new = ('    actual=observe(args.cpp,cases)\n'
                   '    if len(actual) != len(cases):\n'
                   "        raise RuntimeError('Native observations must cover the complete input matrix')\n"
                   '    failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]\n')
            assert previous.count(new) == 1
            previous = previous.replace(new, old, 1)
        assert hashlib.sha256(previous.encode()).hexdigest() == prior['runtime:' + path], path
    current = (ROOT / 'tests/CMakeLists.txt').read_text()
    marker = '\n\n# Complete original generated Q/relationship SQLite drivers and native consumers.\n'
    assert current.count(marker) == 1
    preceding = current.split(marker, 1)[0] + '\n'
    expected = json.loads(gzip.decompress(PREVIOUS.read_bytes()))[
        'changed_existing']['runtime:tests/CMakeLists.txt']['after']
    assert hashlib.sha256(preceding.encode()).hexdigest() == expected
    print('Recovered', len(NAMES), 'preceding typed-Q comparisons and CMake registry byte for byte')


if __name__ == '__main__':
    main()
