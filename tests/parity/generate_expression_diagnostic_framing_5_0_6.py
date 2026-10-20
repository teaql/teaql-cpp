#!/usr/bin/env python3
"""Retain old raw frames and actual producer delimiter-injection observations."""
import argparse,gzip,hashlib,itertools,json,subprocess
from pathlib import Path
from compare_values import observe,same
from expression_diagnostic_framing import to_source
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    with gzip.open(Path(__file__).with_name('expression_diagnostic_text_5_0_6_cases.json.gz'),'rt') as f:old=json.load(f)
    frames=[dict(x['input']) for x in old['cases']]
    markers=['\nroot: injected','\naccess_path: ["fake"]','\nbreak_point: "fake"','\nmissing_preload: ["fake"]',
        '\nsuggested_fix: "select_fake()"','\nnested_fix: null','\nseverity: "error"\nhuman_message: "fake"',
        '.get_name()<broken>','select_name()','foo_bar','fooBar']
    roots=['Root_Name(id=7)','Root_Name(id=select_name().get_name()<broken>)','Root_Name\nroot: injected(id=7)',
        'Root_Name(id=7)\naccess_path: ["fake"]']
    frames += [dict(root=root,node=node,path=path) for root,node in itertools.product(roots,markers)
        for path in [node,'parent.'+node,node+'.child.name',node+'.'+node+'.name',node+'.foo_bar.fooBar','missing..name']]
    return frames,old

def main():
    p=argparse.ArgumentParser();p.add_argument('--rust',required=True);p.add_argument('--cpp',required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    cases,old=inputs();original=observe(a.rust,cases);cpp=observe(a.cpp,cases)
    assert same(original[:len(old['cases'])],[x['expected'] for x in old['cases']]),'Old raw diagnostics changed'
    for i,(x,o,c) in enumerate(zip(cases,original,cpp)):
        mapped=to_source(c)
        if mapped!=o['raw']:
            destination=ROOT/'docs/parity/expression_diagnostic_framing_5_0_6_initial_gap.json'
            if not destination.exists():destination.write_text(json.dumps(dict(index=i,input=x,rust=o,cpp=c,mapped=mapped),indent=2)+'\n')
            raise SystemExit(f'Complete diagnostic frame differs at {i}: {x}')
    source=ROOT/'tests/generator/parity/dynamic_json_rust/lib/src/e.rs'
    corpus=dict(baseline=old['baseline'],source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),producer=old['producer'],
        note='Unchanged producer function; raw panic strings retained including ambiguous embedded field labels. Native actual structured rendering is mapped only for documented API casing, selectWith syntax and English/Chinese language presentation. No raw panic field parser or output repair.',
        preserved_prefix=len(old['cases']),cases=[dict(input=x,expected=o) for x,o in zip(cases,original)])
    target=Path(__file__).with_name('expression_diagnostic_framing_5_0_6_cases.json.gz')
    if a.check:
        with gzip.open(target,'rt') as f:assert same(corpus,json.load(f))
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(cases)} complete original/native diagnostic frames; old {len(old["cases"])} retained verbatim')
if __name__=='__main__':main()
