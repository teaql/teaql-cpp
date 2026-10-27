#!/usr/bin/env python3
"""Replay actual in-memory CLI admission with complete controlled outputs."""

import argparse
from contextlib import redirect_stdout
import gzip
import importlib.util
import io
import json
from pathlib import Path
from types import SimpleNamespace
import sys
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same

COMPARATOR = ROOT / 'tests/parity/compare_inmemory.py'
CORPUS = ROOT / 'tests/parity/inmemory_baseline_5_0_6_cases.json.gz'


def load_comparator():
    spec = importlib.util.spec_from_file_location('inmemory_cli_admission', COMPARATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def replay(module, corpus, observed):
    inputs = [row['input'] for row in corpus['cases']]

    def controlled_observe(_binary, selected, _emulator=()):
        assert same(selected, inputs), 'CLI changed the retained input sequence'
        return observed

    output = io.StringIO()
    with patch.object(module.gzip, 'decompress', return_value=json.dumps(corpus).encode()), \
         patch.object(module.subprocess, 'run', return_value=SimpleNamespace(stdout='64')), \
         patch.object(module, 'observe', controlled_observe), \
         patch.object(sys, 'argv', [str(COMPARATOR), '--cpp', '/controlled/native']), \
         redirect_stdout(output):
        try:
            code = module.main()
        except (RuntimeError, AssertionError) as error:
            return dict(accepted=False, reason=str(error))
    return dict(accepted=code is None or code is False or code == 0,
                reason='' if not code else 'comparison reported mismatches')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--expect-repaired', action='store_true')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical admission evidence')
    module = load_comparator()
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    assert len(corpus['cases']) == 6532
    expected = [row['expected'] for row in corpus['cases']]
    assert replay(module, corpus, expected)['accepted']
    changed = dict(corpus, cases=[dict(row) for row in corpus['cases']])
    changed['cases'][0]['unreviewed'] = True
    controls = dict(extra_case_field=replay(module, changed, expected),
                    native_missing_last=replay(module, corpus, expected[:-1]),
                    native_extra_last=replay(module, corpus, expected + [{'unreviewed': True}]))
    assert all(value['accepted'] is (not args.expect_repaired)
               for value in controls.values())
    result = dict(complete=True, whole_goal_complete=False,
                  scope='Controlled actual CLI admission; prior original Rust and current native runs are separate.',
                  cases=6532, expected_repaired=args.expect_repaired, controls=controls)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('In-memory CLI admission: 6532 cases, 3 mutations',
          'rejected' if args.expect_repaired else 'falsely accepted')


if __name__ == '__main__':
    main()
