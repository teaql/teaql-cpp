#!/usr/bin/env python3
"""Verify retained original producer integrity and replay E value observations."""
import hashlib,json
from pathlib import Path
from compare_values import main
from expression_cli_cases import require_cases
if __name__=='__main__':
    root=Path(__file__).resolve().parents[2]
    corpus=json.loads(Path(__file__).with_name('generated_expression_values_5_0_6_cases.json').read_text())
    require_cases(corpus, 108)
    directory=root/'tests/generator/parity/dynamic_json_rust'
    if not (corpus['producer']==json.loads((directory/'PROVENANCE.json').read_text())):
        raise AssertionError
    if not (hashlib.sha256((root/corpus['producer']['model']).read_bytes()).hexdigest()==corpus['producer']['model_sha256']):
        raise AssertionError
    for name,digest in corpus['producer']['files'].items():
        if not (hashlib.sha256((directory/name).read_bytes()).hexdigest()==digest):
            raise AssertionError(name)
    raise SystemExit(main('generated_expression_values_5_0_6_cases.json','Generated E Value/Null'))
