#!/usr/bin/env python3
"""Compare source fields with explicitly documented C++ syntax/English adaptations."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def camel(name):
    parts=name.split('_')
    return parts[0]+''.join(p[:1].upper()+p[1:] for p in parts[1:])
def adapt(source):
    result=source.copy()
    for key in ['original_expr_with_broken_point','suggested_fix','nested_fix']:
        if result[key] is not None:
            result[key]=re.sub(r'\b(?:get|select)_\w*',lambda m:camel(m[0]),result[key])
            result[key]=re.sub(r'E::(\w+)',lambda m:'E::'+camel(m[1]),result[key])
    if result['nested_fix'] is not None:
        # C++ generated child selections use an explicit With suffix. A leaf's
        # empty-argument selectFoo() remains unchanged.
        result['nested_fix']=re.sub(r'\b(select\w+)\((?!\))',r'\1With(',result['nested_fix'])
    root_name=source['root'].split('(')[0]
    path='.'.join(source['access_path'])
    message=f"Access to {root_name}.{path} requires preloading. Add {result['suggested_fix']}"
    if result['nested_fix'] is not None:message+=f" or the complete nested selection {result['nested_fix']}"
    result['human_message']=message+' to the query.'
    return result
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--cpp',required=True);parser.add_argument('--rust');args=parser.parse_args()
    corpus=json.loads(Path(__file__).with_name('generated_expression_diagnostics_5_0_6_cases.json').read_text())
    assert corpus['baseline']==json.loads((ROOT/'upstream.lock.json').read_text())
    assert corpus['producer']==json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    assert corpus['source_sha256']==hashlib.sha256((ROOT/'tests/generator/parity/dynamic_json_rust/lib/src/e.rs').read_bytes()).hexdigest()
    for path,digest in corpus.get('extra_sources',{}).items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest
    for directory,provenance in corpus['producers'].items():
        assert json.loads((ROOT/directory/'PROVENANCE.json').read_text())==provenance
        assert hashlib.sha256((ROOT/provenance['model']).read_bytes()).hexdigest()==provenance['model_sha256']
        for path,digest in provenance['files'].items():
            assert hashlib.sha256((ROOT/directory/path).read_bytes()).hexdigest()==digest,(directory,path)
    inputs=''.join(json.dumps(c['input'])+'\n' for c in corpus['cases'])
    for binary,adapter in [(args.cpp,adapt)]+([(args.rust,lambda x:x)] if args.rust else []):
        outputs=subprocess.run([binary],input=inputs,text=True,capture_output=True,check=True).stdout.splitlines()
        assert len(outputs)==len(corpus['cases'])
        for index,(line,case) in enumerate(zip(outputs,corpus['cases'])):
            observed=json.loads(line);expected=adapter(case['expected'])
            if observed!=expected:raise SystemExit(f'Case {index}: {case["input"]}\nExpected {expected}\nObserved {observed}')
    print(f'{len(corpus["cases"])} source diagnostic observations match (explicit C++ spelling and English text adaptations)')
