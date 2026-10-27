#!/usr/bin/env python3
"""Recover the preceding helper comparisons and CMake registration byte for byte."""

import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MILESTONE = ROOT / 'docs/parity/query_input_cli_source_milestone_5_0_6.json.gz'
NAMES = (
    'recursive_attachment_helpers',
    'entity_helper_entries',
    'generated_behavior_hooks',
    'generated_reverse_many_helpers',
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def remove_once(data, fragment):
    assert data.count(fragment) == 1, fragment
    return data.replace(fragment, '', 1)


def main():
    prior = json.loads(gzip.decompress(MILESTONE.read_bytes()))['input_hashes']
    rows = []
    for name in NAMES:
        path = 'tests/parity/compare_' + name + '_5_0_6.py'
        current = (ROOT / path).read_text()
        original = remove_once(current, 'from expression_cli_cases import require_cases\n')
        variable = 'data' if name == 'generated_reverse_many_helpers' else 'corpus'
        original = remove_once(original, '        require_cases(' + variable + ', len(inputs), inputs)\n')
        variable = 'actual' if name in ('entity_helper_entries', 'generated_behavior_hooks') else 'native'
        original = remove_once(original, '    if len(' + variable + ') != len(inputs):\n'
                               "        raise RuntimeError('Native observations must cover the complete input matrix')\n")
        assert digest(original.encode()) == prior['runtime:' + path], path
        rows.append(dict(path=path, preceding_sha256=digest(original.encode()),
                         current_sha256=digest(current.encode())))
    path = 'tests/CMakeLists.txt'
    current = (ROOT / path).read_text()
    marker = '\n\n# Complete original generated-helper drivers and exact current native consumers.\n'
    assert current.count(marker) == 1
    original = current.split(marker, 1)[0]
    assert digest(original.encode()) == prior['runtime:' + path], path
    rows.append(dict(path=path, preceding_sha256=digest(original.encode()),
                     current_sha256=digest(current.encode())))
    print('Recovered', len(rows), 'preceding comparison/registration files byte for byte')


if __name__ == '__main__':
    main()
