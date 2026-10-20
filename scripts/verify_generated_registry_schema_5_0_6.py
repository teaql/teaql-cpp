#!/usr/bin/env python3
"""Compare actual standalone generated registry/schema outcomes without normalization."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from verify_runtime_configuration_kernel_5_0_6 import same, verify_inputs
from verify_business_runtime_configuration_5_0_6 import source_inputs

ROOT = Path(__file__).resolve().parents[1]


def observe(executable, cases):
    result = subprocess.run([str(executable.resolve())],
                            input=''.join(json.dumps(case['input']) + '\n' for case in cases),
                            env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'),
                            text=True, capture_output=True, check=True)
    outcomes = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(outcomes) == len(cases), 'Incomplete registry/schema observations'
    return outcomes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    args = parser.parse_args()
    record = json.loads((ROOT / 'docs/parity/generated_registry_schema_original_5_0_6.json').read_text())
    verify_inputs(record)
    source_inputs()  # Verify the complete original model/SDK and reference registry identities.
    assert len(record['cases']) == 13
    expected = [case['source_expected'] for case in record['cases']]
    if args.rust:
        assert same(observe(args.rust, record['cases']), expected), 'Original registry/schema outcomes changed'
    actual = observe(args.cpp, record['cases'])
    for case, value in zip(record['cases'], actual):
        assert same(value, case['source_expected']), (case['input'], value, case['source_expected'])
    print('Verified 13 complete original/generated registry/schema outcomes and real default callbacks')


if __name__ == '__main__':
    main()
