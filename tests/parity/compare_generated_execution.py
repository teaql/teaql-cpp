#!/usr/bin/env python3
"""Verify execution corpus provenance, then compare actual results and SQL logs."""
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import main

ROOT = Path(__file__).resolve().parents[2]

if __name__ == '__main__':
    with gzip.open(Path(__file__).with_name('generated_execution_5_0_6_cases.json.gz'), 'rt') as source:
        corpus = json.load(source)
    producer = json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    if corpus['producer'] != producer:
        raise SystemExit('Generated execution producer provenance differs')
    for name, expected in corpus['fixtures'].items():
        if hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() != expected:
            raise SystemExit(f'Generated execution database fixture changed: {name}')
    raise SystemExit(main('generated_execution_5_0_6_cases.json.gz', 'Generated execution'))
