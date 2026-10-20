#!/usr/bin/env python3
"""Check source clock failures without confusing explicit signed values with now()."""
import argparse
import json
import os
from pathlib import Path
import subprocess

CHRONO_MAX_MILLIS = 8210266876799999


def observe(executable, clock, millis, mode):
    environment = dict(os.environ, LD_PRELOAD=str(Path(clock).resolve()),
                       TEAQL_TEST_SAMPLE_WALL_MILLIS=str(millis))
    result = subprocess.run([str(executable), mode], capture_output=True, text=True, env=environment)
    if millis < 0 and mode != 'negative_value':
        if result.returncode == 0 or result.stdout or 'system time before Unix epoch' not in result.stderr:
            raise RuntimeError(f'Expected pre-epoch clock failure: {executable}, {result}')
        return dict(outcome='failure', reason='system time before Unix epoch',
                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
    if millis > CHRONO_MAX_MILLIS and mode != 'negative_value':
        known_failure = ('System business time exceeds chrono\'s UTC date range' in result.stderr
                         or 'called `Option::unwrap()` on a `None` value' in result.stderr)
        if result.returncode == 0 or result.stdout or not known_failure:
            raise RuntimeError(f'Expected chrono date-range failure: {executable}, {result}')
        return dict(outcome='failure', reason='clock outside chrono UTC date range',
                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
    if result.returncode != 0:
        raise RuntimeError(f'Clock boundary probe failed: {result.stderr}')
    expected = '-1,-1\n' if mode == 'negative_value' else f'{millis}\n'
    if result.stdout != expected:
        raise RuntimeError(f'Clock boundary output differs: {result.stdout!r} != {expected!r}')
    return dict(outcome='value', output=result.stdout, exit_code=result.returncode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--clock', required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('Source comparison evidence requires --rust')
    if args.record and args.record.exists():
        parser.error('Evidence output already exists; use a fresh record path')
    records = []
    for millis in (-1001, -1, 0, 1, 1790812800123,
                   9223372036854, 9223372036855, 10413792000000, 253402300799999,
                   CHRONO_MAX_MILLIS, CHRONO_MAX_MILLIS + 1, 9223372036854775807):
        for mode in ('timestamp', 'utc', 'negative_value'):
            cpp = observe(args.cpp, args.clock, millis, mode)
            rust = observe(args.rust, args.clock, millis, mode) if args.rust else None
            if rust is not None:
                if cpp['outcome'] != rust['outcome'] or (cpp['outcome'] == 'value' and cpp['output'] != rust['output']):
                    raise RuntimeError('Clock boundary source/native outcomes differ')
            records.append(dict(wall_millis=millis, mode=mode, cpp=cpp, rust=rust))
    if args.record:
        args.record.write_text(json.dumps(dict(cases=records,
            scope='Clock success values compare exactly. Rust panic and C++ exception are separate language failure mechanisms; neither yields a successful timestamp. Explicit signed values remain supported.'), indent=2) + '\n')
    print(f'Clock epoch boundary: {len(records)}/{len(records)} outcomes verified')


if __name__ == '__main__':
    main()
