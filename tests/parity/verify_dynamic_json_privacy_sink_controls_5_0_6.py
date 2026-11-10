#!/usr/bin/env python3
"""Keep generated private SQL sink checks active under optimized Python."""

import argparse
import ast
import os
from pathlib import Path
import subprocess
import sys
import tempfile


VERIFIER = Path(__file__).with_name('verify_dynamic_json_privacy_sinks_5_0_6.py')


def run(executable, optimized, extra):
    command = [sys.executable]
    if optimized:
        command.append('-O')
    command.extend([str(VERIFIER), '--cpp', str(executable), *extra])
    environment = dict(os.environ)
    environment.pop('PYTHONOPTIMIZE', None)
    return subprocess.run(command, capture_output=True, text=True, env=environment)


def require_result(result, expected_success, label):
    if (result.returncode == 0) != expected_success:
        raise RuntimeError(
            f'{label}: exit {result.returncode}; stdout={result.stdout}; stderr={result.stderr}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True, type=Path)
    args = parser.parse_args()
    executable = args.cpp.resolve(strict=True)
    source = ast.parse(VERIFIER.read_text())
    if any(isinstance(node, ast.Assert) for node in ast.walk(source)):
        raise RuntimeError('Generated private SQL sink verifier has removable assertions')

    with tempfile.TemporaryDirectory(prefix='teaql_privacy_sink_controls_506_') as temporary:
        sentinel = Path(temporary) / 'retained.json.gz'
        sentinel.write_bytes(b'original evidence must remain unchanged')
        original = sentinel.read_bytes()

        for optimized in (False, True):
            mode = 'optimized' if optimized else 'normal'
            require_result(run(executable, optimized, []), True, mode + ' valid policy')

            mismatch = run(executable, optimized, ['--policy', 'plain'])
            require_result(mismatch, False, mode + ' incorrect plaintext policy')
            if 'ordinary sink policy changed' not in mismatch.stderr:
                raise RuntimeError(mode + ': plaintext mismatch failed for an unrelated reason')

            overwrite = run(executable, optimized, ['--output', str(sentinel)])
            require_result(overwrite, False, mode + ' existing evidence')
            if 'Refusing to replace existing sink evidence' not in overwrite.stderr:
                raise RuntimeError(mode + ': evidence overwrite failed for an unrelated reason')
            if sentinel.read_bytes() != original:
                raise RuntimeError(mode + ': existing evidence was modified')

    print('Generated private SQL sink: 6 normal/optimized acceptance and rejection controls passed')


if __name__ == '__main__':
    main()
