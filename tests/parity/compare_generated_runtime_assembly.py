#!/usr/bin/env python3
"""Replay ordinary module/root assembly against complete retained source fixtures."""
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import main
if __name__=='__main__':
    root=Path(__file__).resolve().parents[2]
    corpus=json.loads(gzip.decompress(Path(__file__).with_name('generated_runtime_assembly_5_0_6_cases.json.gz').read_bytes()))
    for name,fixture in [('checkers','checkers'),('inheritance','inheritance'),('relations','relations')]:
        directory=root/'tests/generator/parity'/(fixture+'_rust')
        producer=corpus['producers'][name]
        assert producer==json.loads((directory/'PROVENANCE.json').read_text())
        assert hashlib.sha256((root/producer['model']).read_bytes()).hexdigest()==producer['model_sha256']
        for file,digest in producer['files'].items():
            assert hashlib.sha256((directory/file).read_bytes()).hexdigest()==digest,file
    raise SystemExit(main('generated_runtime_assembly_5_0_6_cases.json.gz','Generated runtime assembly'))
