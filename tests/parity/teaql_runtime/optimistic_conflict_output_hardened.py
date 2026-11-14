"""Replay the pinned isolated-process optimistic-conflict output contract."""
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--cpp', required=True)
parser.add_argument('--cpp-emulator', default='')
args = parser.parse_args()
emulator = [part for part in args.cpp_emulator.split(';') if part]
result = subprocess.run(emulator + [args.cpp, 'mutation-only'], capture_output=True, check=False)
if not (result.returncode == 0):
    raise AssertionError((result.returncode, result.stdout, result.stderr))
if not (b'running 1 test' in result.stdout):
    raise AssertionError(result.stdout)
for output in (result.stdout, result.stderr):
    if not (b'OptimisticLockConflict in memory.rs' not in output):
        raise AssertionError(output)
print('Original isolated-process optimistic-conflict output contract passed')
