#!/usr/bin/env python3
"""Run the generated direct-filename contracts in an isolated filesystem fixture."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off')
    with tempfile.TemporaryDirectory(prefix='teaql_filename_contract_') as directory:
        subprocess.run([str(args.cpp.resolve()), directory], env=environment, check=True)


if __name__ == '__main__':
    main()
