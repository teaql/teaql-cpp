#!/usr/bin/env python3
"""Reject actual metadata JSON kind corruptions at the Python source boundary."""
import argparse
import ast
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
PREVIOUS_COMPARATOR_COMMIT='49fe770cd023fa401adf20a1576420b759ae1099'
sys.path.insert(0,str(ROOT/'scripts'))
from verify_audit_metadata_source_5_0_6 import same_contract


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    assert not args.record.exists(),'Preserve earlier results'
    receipt=ROOT/'docs/parity/audit_builtin_installed_source_5_0_6.json.gz'
    source=json.loads(gzip.decompress(receipt.read_bytes()));assert source['complete'] is True
    old_text=subprocess.check_output(['git','show',PREVIOUS_COMPARATOR_COMMIT+':scripts/verify_audit_metadata_source_5_0_6.py'],cwd=ROOT,text=True)
    function=next(n for n in ast.parse(old_text).body if isinstance(n,ast.FunctionDef) and n.name=='same_contract')
    namespace={};exec(compile(ast.Module(body=[function],type_ignores=[]),'<retained metadata comparison>','exec'),namespace)
    old=namespace['same_contract']
    valid=next(c['original'] for c in source['cases'] if c['original']['parsed'] and c['original']['indexed'] is False)
    related=next(c['original'] for c in source['cases'] if c['original']['parsed'] and c['original']['relations'] and c['original']['relations'][0]['many'] is True)
    assert same_contract(valid,copy.deepcopy(valid))
    assert same_contract({'parsed':False,'stage':'lexical'},{'parsed':False,'stage':'generation'})
    cases=[('indexed_bool_to_number',valid,dict(valid,indexed=0)),
           ('parsed_bool_to_number',valid,dict(valid,parsed=1)),
           ('invalid_source_parsed_number',dict(valid,parsed=1),valid)]
    changed=copy.deepcopy(related);changed['relations'][0]['many']=1
    cases.append(('nested_many_bool_to_number',related,changed))
    cases += [('missing_metadata_field',valid,{k:v for k,v in valid.items() if k!='indexed'}),
              ('extra_metadata_field',valid,dict(valid,extra=0)),
              ('invalid_refusal_number',{'parsed':False},{'parsed':0}),
              ('invalid_expected_refusal_number',{'parsed':0},{'parsed':False})]
    controls=[]
    for name,expected,actual in cases:
        assert not same_contract(expected,actual),name
        controls.append(dict(name=name,expected=expected,actual=actual,
                             previous_accepted=old(expected,actual),current_rejected=True))
    assert sum(c['previous_accepted'] for c in controls)==5
    output=dict(complete=True,scope=__doc__,controls=controls,
                previous_comparator_commit=PREVIOUS_COMPARATOR_COMMIT,
                source_receipt_sha256=hashlib.sha256(receipt.read_bytes()).hexdigest(),
                comparator_sha256=hashlib.sha256((ROOT/'scripts/verify_audit_metadata_source_5_0_6.py').read_bytes()).hexdigest(),
                verifier_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.record.write_bytes(gzip.compress(json.dumps(output).encode(),mtime=0))
    print('Eight metadata corruptions reject; five actually accepted by the previous comparison')


if __name__=='__main__':main()
