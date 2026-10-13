#!/usr/bin/env python3
"""Check aggregation evidence against its locked producer and actual C++ probe."""
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import main
ROOT = Path(__file__).resolve().parents[2]
if __name__ == '__main__':
    with gzip.open(Path(__file__).with_name('generated_aggregation_5_0_6_cases.json.gz'), 'rt') as source:
        corpus = json.load(source)
    producer = json.loads((ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    if corpus['producer'] != producer:
        raise SystemExit('Aggregation producer provenance differs')
    for name, digest in corpus['fixture_hashes'].items():
        if hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest() != digest:
            raise SystemExit(f'Aggregation database fixture changed: {name}')
    raise SystemExit(main('generated_aggregation_5_0_6_cases.json.gz', 'Generated aggregation'))
