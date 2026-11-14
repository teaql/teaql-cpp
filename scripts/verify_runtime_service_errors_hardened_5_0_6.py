#!/usr/bin/env python3
"""Compare complete generated service-error diagnostics with original observations."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from verify_runtime_configuration_kernel_5_0_6 import load_original_record, same

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
    record = load_original_record('runtime_service_errors_original_observations_5_0_6.json')
    if not (len(record['cases']) == 5):
        raise AssertionError
    if args.rust:
        if not (same(observe(args.rust), record['cases'])):
            raise AssertionError('Original service-error observations changed')
    if not (same(observe(args.cpp), record['cases'])):
        raise AssertionError('C++ service-error diagnostics differ')
    print('Verified all five original service-error Display/Debug/source/source-Debug branches')


if __name__ == '__main__':
    main()
