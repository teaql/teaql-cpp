#!/usr/bin/env python3
"""Verify retained original producer integrity and replay E value observations."""
import hashlib,json
from pathlib import Path
from compare_values import main
if __name__=='__main__':
    root=Path(__file__).resolve().parents[2]
    corpus=json.loads(Path(__file__).with_name('generated_expression_values_5_0_6_cases.json').read_text())
    directory=root/'tests/generator/parity/dynamic_json_rust'
    assert corpus['producer']==json.loads((directory/'PROVENANCE.json').read_text())
    assert hashlib.sha256((root/corpus['producer']['model']).read_bytes()).hexdigest()==corpus['producer']['model_sha256']
    for name,digest in corpus['producer']['files'].items():
        assert hashlib.sha256((directory/name).read_bytes()).hexdigest()==digest,name
    raise SystemExit(main('generated_expression_values_5_0_6_cases.json','Generated E Value/Null'))
