#!/usr/bin/env python3
"""Compare complete generated service-error diagnostics with original observations."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from verify_runtime_configuration_kernel_5_0_6 import verify_inputs, same

ROOT = Path(__file__).resolve().parents[1]


def observe(executable):
    environment = dict(os.environ)
    environment['TEAQL_LOG_ENDPOINT'] = 'off'
    output = subprocess.check_output([str(executable.resolve())], env=environment, text=True)
    return json.loads(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    args = parser.parse_args()
    record = json.loads((ROOT / 'docs/parity/runtime_service_errors_original_observations_5_0_6.json').read_text())
    verify_inputs(record)
    assert len(record['cases']) == 5
    if args.rust:
        assert same(observe(args.rust), record['cases']), 'Original service-error observations changed'
    assert same(observe(args.cpp), record['cases']), 'C++ service-error diagnostics differ'
    print('Verified all five original service-error Display/Debug/source/source-Debug branches')


if __name__ == '__main__':
    main()
