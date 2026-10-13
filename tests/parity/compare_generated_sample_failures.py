#!/usr/bin/env python3
"""Compare original sample failures and surviving rows without error or result normalization."""
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import main
if __name__ == '__main__':
    root = Path(__file__).resolve().parents[2]
    directory = root / 'tests/generator/parity/relations_rust'
    corpus = json.loads(gzip.decompress(Path(__file__).with_name('generated_sample_failures_5_0_6_cases.json.gz').read_bytes()))
    assert corpus['producer'] == json.loads((directory / 'PROVENANCE.json').read_text())
    assert hashlib.sha256((root / corpus['producer']['model']).read_bytes()).hexdigest() == corpus['producer']['model_sha256']
    for file, digest in corpus['producer']['files'].items():
        assert hashlib.sha256((directory / file).read_bytes()).hexdigest() == digest, file
    raise SystemExit(main('generated_sample_failures_5_0_6_cases.json.gz', 'Generated sample failures'))
