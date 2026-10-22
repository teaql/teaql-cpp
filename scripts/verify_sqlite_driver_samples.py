#!/usr/bin/env python3
"""Run and verify the optional matched SQLite driver sampling harness."""
import argparse
import gzip
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--build-type', required=True)
    args = parser.parse_args()
    command = [str(args.cpp.resolve()), '--measure']
    result = subprocess.run(command, env={**os.environ, 'TEAQL_LOG_ENDPOINT': 'off'},
                            text=True, capture_output=True)
    record = {'command': command, 'build_type': args.build_type,
              'exit_code': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr,
              'binary_sha256': hashlib.sha256(args.cpp.read_bytes()).hexdigest(),
              'performance_claim': False}
    try:
        assert result.returncode == 0, 'candidate process failed'
        lines = [line.split(',') for line in result.stdout.splitlines()]
        shapes = set(itertools.product(('0', '1'), ('0', '1'), ('1', '100', '10000')))
        first = [line for line in lines if line[0] == 'FIRST_RUNTIME_DEFAULT_LOG']
        assert len(first) == 12 and {tuple(row[1:4]) for row in first} == shapes
        assert all(len(row) == 5 and int(row[4]) > 0 for row in first)
        samples = [line for line in lines if line[0] == 'SAMPLE']
        expected = {shape + (str(i), lane) for shape in shapes
                    for i in range(31) for lane in ('native', 'runtime')}
        assert len(samples) == len(expected) == 744
        assert {tuple(row[1:6]) for row in samples} == expected
        for row in samples:
            assert len(row) == 11 and all(int(v) >= 0 for v in row[6:])
            if row[5] == 'runtime':
                assert int(row[7]) > 0 and int(row[8]) > 0
            else:
                assert int(row[7]) == 0 and int(row[9]) > 0 and int(row[10]) > 0
        stages = ('compact_fetch', 'plain_compact_decode', 'runtime_compact_decode',
                  'first_state_clone', 'repeat_state_clone')
        layers = [line for line in lines if line[0] == 'LAYER']
        assert len(layers) == 60
        assert {tuple(row[1:5]) for row in layers} == {
            shape + (stage,) for shape in shapes for stage in stages}
        assert all(len(row) == 9 and all(int(v) >= 0 for v in row[5:]) for row in layers)
        summaries = [line for line in lines if line[0] == 'SUMMARY']
        assert len(summaries) == 24
        assert {tuple(row[1:5]) for row in summaries} == {
            shape + (lane,) for shape in shapes for lane in ('native', 'runtime')}
        for row in summaries:
            assert len(row) == 13 and row[5] == '31'
            selected = [s for s in samples if s[1:4] == row[1:4] and s[5] == row[4]]
            times = sorted(int(s[6]) for s in selected)
            assert int(row[6]) == times[15] and int(row[7]) == times[29]
            assert math.isclose(float(row[8]), 31_000_000_000 / sum(times), rel_tol=1e-5)
            allocations = sorted(tuple(int(v) for v in s[7:11]) for s in selected)
            assert tuple(int(v) for v in row[9:13]) == allocations[15]
        record.update(coverage_valid=True, raw_samples=744, layer_observations=60,
                      summaries=24, first_runtime_observations=12)
    except (AssertionError, ValueError, IndexError) as error:
        record.update(coverage_valid=False, validation_error=str(error))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(args.output, 'wt') as stream:
        json.dump(record, stream, indent=2)
    print(json.dumps({key: value for key, value in record.items()
                      if key not in ('stdout', 'stderr')}))
    return 0 if record['coverage_valid'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
