#!/usr/bin/env python3
"""Require the exact previous parser to fail only the new permanent corpus test."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
PRODUCER=ROOT.parent/'teaql_code_gen_cpp'
RELATIVE='generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--comparison',type=Path,required=True)
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists()
    initial=json.loads(gzip.decompress((ROOT/'docs/parity/audit_grammar_branch_initial_5_0_6.json.gz').read_bytes()))
    old=subprocess.check_output(['git','-C',str(PRODUCER),'show','HEAD:'+RELATIVE])
    expected=initial['input_hashes_before'][str(PRODUCER/RELATIVE)]
    assert hashlib.sha256(old).hexdigest()==expected
    record=dict(complete=False,scope=__doc__,previous_parser_sha256=expected,steps=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_builtin_old_junit_506_') as temporary:
            work=Path(temporary);record['owned_directory']=str(work)
            source=work/'CppRustLiteralMetadata.java';source.write_bytes(old)
            command=['python3','scripts/verify_audit_metadata_source_5_0_6.py','--candidate-parser',str(source),'--record',str(args.comparison)]
            result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True)
            record['steps'].append(dict(command=command,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
            assert result.returncode==1, 'The old source/JUnit gate must actually fail'
            observed=json.loads(gzip.decompress(args.comparison.read_bytes()))
            assert observed['inputs_unchanged'] and observed['owned_directory_removed']
            assert len(observed['cases'])==8594 and len(observed['differences'])==45
            assert not observed['original_corpus_differences'] and not observed['junit_passed']
            junit=next(s for s in observed['steps'] if s['label']=='junit')
            assert junit['exit_code']==1
            summary=re.search(r'Tests run: (\d+),\s+Failures: 1\b',junit['stdout']);assert summary
            assert 'witnessedSyntaxBranchesPreserveCompleteSourceMetadata' in junit['stdout']
            record['junit_tests']=int(summary[1]);record['junit_failures']=1;record['metadata_differences']=45
            record['comparison_sha256']=hashlib.sha256(args.comparison.read_bytes()).hexdigest()
            record['owned_bytes_removed']=sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record['complete']=True
    finally:
        record['owned_directory_removed']=not Path(record.get('owned_directory','/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(),mtime=0))
    print('Previous parser reproduces45 effects and fails only the new corpus among',record['junit_tests'],'tests')


if __name__=='__main__':main()
