#!/usr/bin/env python3
"""Replay locked-source catalog observations and verify the generated assets."""
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import main


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[2]
    corpus = json.loads(gzip.decompress(
        Path(__file__).with_name('generated_i18n_5_0_6_cases.json.gz').read_bytes()))
    for name, expected in corpus['fixtures'].items():
        if hashlib.sha256((root / name).read_bytes()).hexdigest() != expected:
            raise SystemExit(f'Generated catalog differs from its source observation: {name}')
    raise SystemExit(main('generated_i18n_5_0_6_cases.json.gz', 'Generated i18n'))
