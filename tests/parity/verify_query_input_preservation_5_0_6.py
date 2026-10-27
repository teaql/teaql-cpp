#!/usr/bin/env python3
"""Reconstruct preceding query-input CLIs and reproduce their 24 false passes."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import types

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from verify_query_input_admission_5_0_6 import NAMES, replay

PRECEDING = ROOT / 'docs/parity/lifecycle_cli_source_milestone_5_0_6.json.gz'
CASE_GUARD = '        require_cases(corpus, len(cases), cases)\n'
LENGTH_GUARD = (
    '    if len(actual) != len(cases):\n'
    "        raise RuntimeError('Native observations must cover the complete input matrix')\n"
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def preceding_module(name, old_hash):
    relative = 'tests/parity/compare_' + name + '_5_0_6.py'
    path = ROOT / relative
    text = path.read_text()
    assert text.count('from expression_cli_cases import require_cases\n') == 1
    assert text.count(CASE_GUARD) == 1 and text.count(LENGTH_GUARD) == 1
    preceding = (text.replace('from expression_cli_cases import require_cases\n', '', 1)
                 .replace(CASE_GUARD, '', 1).replace(LENGTH_GUARD, '', 1))
    assert digest(preceding.encode()) == old_hash, name + ': preceding comparator bytes'
    module = types.ModuleType('preceding_' + name)
    module.__file__ = str(path)
    exec(compile(preceding, str(path), 'exec'), module.__dict__)
    return relative, module


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical preservation evidence')
    preceding = json.loads(gzip.decompress(PRECEDING.read_bytes()))
    original_cmake = preceding['input_hashes']['runtime:tests/CMakeLists.txt']
    cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
    marker = '\n\n# Complete original query-input matrices and every actual CTest consumer.'
    assert cmake.count(marker) == 1
    assert digest(cmake.split(marker, 1)[0].encode()) == original_cmake
    rows = []
    owned = None
    with tempfile.TemporaryDirectory(prefix='teaql_query_input_preservation_506_') as dirname:
        owned = Path(dirname)
        for name in NAMES:
            relative = 'tests/parity/compare_' + name + '_5_0_6.py'
            old_hash = preceding['input_hashes']['runtime:' + relative]
            path, module = preceding_module(name, old_hash)
            corpus = json.loads(gzip.decompress(module.CORPUS.read_bytes()))
            inputs = module.inputs()
            assert len(corpus['cases']) == len(inputs)
            expected = [case['expected'] for case in corpus['cases']]
            valid = replay(module, module.CORPUS, inputs, expected)
            assert valid['accepted']
            changed = dict(corpus)
            changed['cases'] = corpus['cases'].copy()
            changed['cases'][0] = dict(changed['cases'][0], extra_case_field='unreviewed')
            mutant = owned / (name + '.json.gz')
            mutant.write_bytes(gzip.compress(json.dumps(changed, ensure_ascii=False).encode(),
                                            mtime=0))
            extra_case = replay(module, mutant, inputs, expected)
            missing_native = replay(module, module.CORPUS, inputs, expected[:-1])
            extra_native = replay(module, module.CORPUS, inputs,
                                  expected + [{'unreviewed_output': True}])
            assert all(row['accepted'] for row in (extra_case, missing_native, extra_native))
            rows.append(dict(name=name, comparator=path, preceding_sha256=old_hash,
                             current_sha256=digest((ROOT / relative).read_bytes()),
                             cases=len(inputs), valid=valid, extra_case=extra_case,
                             missing_native=missing_native, extra_native=extra_native))
        owned_bytes = sum(path.stat().st_size for path in owned.rglob('*') if path.is_file())
    assert owned is not None and not owned.exists()
    record = dict(complete=True, whole_goal_complete=False,
                  scope='Exact preceding comparator bytes, controlled outputs and 24 reproduced '
                        'false acceptances. Rust source and current native execution are separate.',
                  baseline_sha256=digest(PRECEDING.read_bytes()),
                  preceding_cmake_sha256=original_cmake,
                  rows=rows, owned_directory=str(owned),
                  owned_logical_bytes_removed=owned_bytes,
                  owned_directory_removed=True)
    if args.record is not None:
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(),
                                           mtime=0, compresslevel=3))
    print('Query-input preservation: 8 preceding comparators reconstructed;',
          len(rows) * 3, 'false acceptances reproduced')


if __name__ == '__main__':
    main()
