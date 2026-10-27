#!/usr/bin/env python3
"""Compile actual original/C++ metadata callers with the qualified builtin overlay."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
CASES=['expr_verbatim_direct','builtin_empty_direct','builtin_payload_nested','builtin_trivia_direct',
       'builtin_method_body','builtin_attribute_direct','builtin_doc_direct',
       'builtin_invalid_braces_direct','builtin_plain_control_direct']


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--comparison',type=Path,required=True)
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists()
    proposal_path=ROOT/'docs/parity/audit_grammar_builtin_proposal_5_0_6.json.gz'
    proposal=json.loads(gzip.decompress(proposal_path.read_bytes()));assert proposal['complete'] is True
    candidate=proposal['candidate_source'];assert hashlib.sha256(candidate.encode()).hexdigest()==proposal['candidate_sha256']
    record=dict(complete=False,scope=__doc__,proposal_sha256=hashlib.sha256(proposal_path.read_bytes()).hexdigest(),steps=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_builtin_callers_506_') as temporary:
            work=Path(temporary);record['owned_directory']=str(work)
            source=work/'CppRustLiteralMetadata.java';source.write_text(candidate)
            corpus=ROOT.parent/'teaql_code_gen_cpp/generator/src/test/resources/cpp_grammar_branch_metadata_5_0_6.json.gz'
            command=['python3','scripts/verify_audit_metadata_compiler_5_0_6.py','--candidate-parser',str(source),
                     '--corpus',str(corpus),'--cases',*CASES,'--record',str(args.comparison)]
            result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True)
            record['steps'].append(dict(command=command,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
            result.check_returncode()
            observed=json.loads(gzip.decompress(args.comparison.read_bytes()));assert observed['complete'] is True
            record['comparison_sha256']=hashlib.sha256(args.comparison.read_bytes()).hexdigest()
            record['owned_bytes_removed']=sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record['complete']=True
    finally:
        record['owned_directory_removed']=not Path(record.get('owned_directory','/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(),mtime=0))
    print('Builtin caller proposal passes nine actual original/C++ entity/descriptor/field-policy pairs')


if __name__=='__main__':main()
