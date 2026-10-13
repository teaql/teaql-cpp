#!/usr/bin/env python3
"""Capture original sample save failures, retained prior commits and failed reference loads."""
import gzip
import hashlib
import itertools
import json
import sys
from pathlib import Path
from compare_values import observe
ROOT = Path(__file__).resolve().parents[2]
inputs = [dict(root=True, scale='Tiny', seed=7, rounds=rounds,
               fault_model=model, fail_at=fail_at)
          for model, fail_at, rounds in itertools.product(
              ['device', 'profile', 'school'], [1, 3, 20], [1, 2])]
inputs.extend(dict(root=False, scale='Tiny', seed=7, rounds=rounds,
                   fault_model='device', fail_at=fail_at)
              for fail_at, rounds in itertools.product([1, 3, 20], [1, 2]))
inputs.extend(dict(root=root, scale='Tiny', seed=0, rounds=rounds,
                   root_query_error=True)
              for root, rounds in itertools.product([False, True], [1, 2]))
directory = ROOT / 'tests/generator/parity/relations_rust'
producer = json.loads((directory / 'PROVENANCE.json').read_text())
for file, digest in producer['files'].items():
    assert hashlib.sha256((directory / file).read_bytes()).hexdigest() == digest, file
expected = observe(sys.argv[1], inputs)
lock = json.loads((ROOT / 'upstream.lock.json').read_text())
corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
              scope='Original generated entry over equal explicit SQLite tables and trigger stimuli; exact errors and complete persisted rows, including subsequent invocations',
              cases=[dict(input=i, expected=e) for i, e in zip(inputs, expected)])
Path(__file__).with_name('generated_sample_failures_5_0_6_cases.json.gz').write_bytes(
    gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
print(f'Captured {len(inputs)} original sample failure/load observations / {sum(len(e) for e in expected)} invocations')
