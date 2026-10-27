#!/usr/bin/env python3
"""Compare complete original panic bytes after explicit native presentation mapping."""
import argparse,gzip,hashlib,json
from pathlib import Path
from compare_values import observe,same
from expression_diagnostic_framing import to_source,verify_negative_controls
from expression_cli_cases import require_cases
ROOT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--cpp',required=True);p.add_argument('--rust');a=p.parse_args()
    with gzip.open(Path(__file__).with_name('expression_diagnostic_framing_5_0_6_cases.json.gz'),'rt') as f:corpus=json.load(f)
    assert corpus['baseline']==json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    assert corpus['source_sha256']==hashlib.sha256((ROOT/'tests/generator/parity/dynamic_json_rust/lib/src/e.rs').read_bytes()).hexdigest()
    assert corpus['producer']==json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    rows=require_cases(corpus,1200)
    cases=[x['input'] for x in rows];expected=[x['expected'] for x in rows]
    if a.rust:
        if not same(expected,observe(a.rust,cases)):
            raise RuntimeError('Raw original panic observations changed')
    native=observe(a.cpp,cases)
    control=cases.index(dict(root='Person(id=7)',node='name',path='parent.name'))
    assert verify_negative_controls(native[control],expected[control]['raw'])==7

    for i,(x,e,c) in enumerate(zip(cases,expected,native)):
        if to_source(c)!=e['raw']:raise SystemExit(f'Whole diagnostic framing differs at {i}: {x}')
    print(f'Compared {len(cases)} complete source/native diagnostic frames (explicit presentation adapter; no panic parser)')
if __name__=='__main__':main()
