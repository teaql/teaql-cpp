#!/usr/bin/env python3
"""Replay original sample helper observations with retained-producer integrity checks."""
import hashlib,json
from pathlib import Path
from compare_values import main
from expression_cli_cases import require_cases
if __name__=='__main__':
    root=Path(__file__).resolve().parents[2]; directory=root/'tests/generator/parity/checkers_rust'
    corpus=json.loads(Path(__file__).with_name('generated_sample_state_5_0_6_cases.json').read_text())
    require_cases(corpus, 144)
    if not (corpus['producer']==json.loads((directory/'PROVENANCE.json').read_text())):
        raise AssertionError
    if not (hashlib.sha256((root/corpus['producer']['model']).read_bytes()).hexdigest()==corpus['producer']['model_sha256']):
        raise AssertionError
    for file,digest in corpus['producer']['files'].items():
        if not (hashlib.sha256((directory/file).read_bytes()).hexdigest()==digest):
            raise AssertionError(file)
    raise SystemExit(main('generated_sample_state_5_0_6_cases.json','Generated sample state'))
