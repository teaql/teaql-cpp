#!/usr/bin/env python3
"""A synchronous log callback cannot reclaim its active Context storage."""
import argparse
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cpp', required=True)
args = parser.parse_args()
result = subprocess.run([args.cpp, 'close'], capture_output=True, text=True, timeout=30)
assert result.returncode == 86, (result.returncode, result.stdout, result.stderr)
print('Sample log callback close: protected termination before arena reclamation')
