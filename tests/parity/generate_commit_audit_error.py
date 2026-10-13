#!/usr/bin/env python3
"""Record exact public after-commit error Display/Debug/default source behavior."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    messages = ['', 'sink failed', 'SECRET-PAYLOAD', '雪\n\t"\\', 'x' * 4096]
    inputs = [dict(message=message, depth=depth)
              for message in messages for depth in [0, 1, 2, 5]]
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    expected = observe(args.rust, inputs)
    actual = observe(args.cpp, inputs)
    for index, (original, result) in enumerate(zip(expected, actual)):
        if not same(original, result):
            mismatch = Path('/tmp/teaql_commit_audit_error_mismatch.json')
            mismatch.write_text(json.dumps(dict(index=index, input=inputs[index],
                                                rust=original, cpp=result), indent=2))
            raise SystemExit(f'Error mismatch {index}; see {mismatch}')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    source = 'teaql-runtime/src/error.rs'
    corpus = dict(
        baseline=lock['commit'], version=lock['version'], source=source,
        source_sha256=hashlib.sha256((ROOT / '.reference/teaql-rs' / source).read_bytes()).hexdigest(),
        note=('Direct unchanged public RuntimeError construction/Display/Debug/Error::source, '
              'including nested after-commit payloads. Queue behavior has separate original '
              'unit and native tests.'),
        cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
    Path(__file__).with_name('commit_audit_error_5_0_6_cases.json').write_text(
        json.dumps(corpus, indent=2) + '\n')
    print(f'Compared and recorded {len(inputs)} after-commit error observations')


if __name__ == '__main__':
    main()
