#!/usr/bin/env python3
"""Capture actual original temporal/scalar/reference defaults and ordinary JSON sample failures."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
from sample_clock_driver import observe_fixed
ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--rust', required=True)
parser.add_argument('--clock', required=True)
args = parser.parse_args()
clocks = [1735689600123, 1709208000456, 946684800789, 123]
inputs = [dict(kind=kind, wall_millis=clock, root=root, scale='Tiny', seed=seed, rounds=1)
          for kind, clock, root, seed in itertools.product(
              ['temporal', 'json_runtime'], clocks, [False, True], [0, 7, 1094, 1095, 18446744073709551596])]
inputs.extend(dict(kind='temporal', wall_millis=clock, root=True, scale='Tiny', seed=7, rounds=2)
              for clock in clocks)
inputs.append(dict(kind='temporal', wall_millis=1735689600123, root=True, scale='Small', seed=1094, rounds=1))
producers = {}
for kind, fixture in [('temporal', 'sample_fields_temporal'), ('json_runtime', 'sample_fields_runtime')]:
    directory = ROOT / 'tests/generator/parity' / (fixture + '_rust')
    provenance = json.loads((directory / 'PROVENANCE.json').read_text())
    for file, digest in provenance['files'].items():
        assert hashlib.sha256((directory / file).read_bytes()).hexdigest() == digest, file
    producers[kind] = provenance
expected = observe_fixed(args.rust, inputs, args.clock)
lock = json.loads((ROOT / 'upstream.lock.json').read_text())
corpus = dict(baseline=lock['commit'], version=lock['version'], producers=producers,
              clock_fixture_sha256=hashlib.sha256((ROOT / 'tests/support/fixed_sample_clock.cpp').read_bytes()).hexdigest(),
              scope='Unchanged original sample entries over equal explicit test schemas and fixed process wall clock; full rows/reports/errors, including signed/unsigned default arithmetic and reference pools',
              cases=[dict(input=i, expected=e) for i, e in zip(inputs, expected)])
Path(__file__).with_name('generated_sample_fields_5_0_6_cases.json.gz').write_bytes(
    gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
print(f'Captured {len(inputs)} original sample-field observations')
