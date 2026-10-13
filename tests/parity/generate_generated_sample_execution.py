#!/usr/bin/env python3
"""Capture unchanged generated sample execution with explicit SQLite test schemas."""
import gzip
import hashlib
import itertools
import json
import sys
from pathlib import Path
from compare_values import observe
ROOT = Path(__file__).resolve().parents[2]
inputs = [dict(root=root, scale=scale, seed=seed, rounds=rounds)
          for root, scale, seed, rounds in itertools.product(
              [False, True], ['Tiny', 'Small'], [0, 7], [1, 2])]
inputs.extend(dict(root=root, scale='Medium', seed=0, rounds=1) for root in [False, True])
producer_dir = ROOT / 'tests/generator/parity/relations_rust'
producer = json.loads((producer_dir / 'PROVENANCE.json').read_text())
for file, digest in producer['files'].items():
    assert hashlib.sha256((producer_dir / file).read_bytes()).hexdigest() == digest, file
expected = observe(sys.argv[1], inputs)
lock = json.loads((ROOT / 'upstream.lock.json').read_text())
corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
              scope='Actual original sample entry; identical explicit test schemas; automatic schema, temporal and numeric models remain separate gates',
              cases=[dict(input=i, expected=e) for i, e in zip(inputs, expected)])
payload = (json.dumps(corpus, indent=2) + '\n').encode()
Path(__file__).with_name('generated_sample_execution_5_0_6_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
print(f'Captured {len(inputs)} original sample executions / {sum(len(e) for e in expected)} rounds')
